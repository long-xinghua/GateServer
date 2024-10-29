#include "MysqlDao.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize) 
	:url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize) {
	try {
		for (int i = 0; i < poolSize_; i++) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();	// 获取mysql驱动实例，通过这个驱动来创建连接
			auto* con = driver->connect(url_, user_, pass_);
			con->setSchema(schema_);	// 绑定一下数据库名字
			// 获取当前时间戳
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			long long timeStamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();	// 将单位转换为秒
			pool_.push(std::make_unique<SqlConnection>(con, timeStamp));	// make_unique会自动在堆区创建一个SqlConnection对象并返回unique_ptr的智能指针
		}

		_check_thread = std::thread([this]() {
			while (!b_stop_) {	// 连接池还没关闭就一直循环
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});

		_check_thread.detach();	// 将线程分离，在后台运行
	}
	catch(sql::SQLException e){
		std::cout << "mysqlPool init failed, error is: " << e.what() << std::endl;
	}
}

MysqlPool::~MysqlPool() {
	std::unique_lock<std::mutex> lock(mutex_);
	//close();
	while(!pool_.empty()) {
		pool_.pop();
	}
}

void MysqlPool::checkConnection() {
	std::lock_guard<std::mutex> guard(mutex_);
	int poolSize = poolSize_;
	//获取当前时间戳
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	long long timeStamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();	// 将单位转换为秒
	for (int i = 0; i < poolSize; i++) {
		auto con = std::move(pool_.front());
		pool_.pop();
		// 创建一个Defer对象，实现类似go语言中的defer功能，此处在for语句结束之前一定会执行lambda表达式将拿出来的连接con塞回pool_
		Defer defer([this, &con]() {
			pool_.push(std::move(con));
			});
		if (timeStamp - con->_last_oper_time < 300) {	// 如果距离上次从操作时间小于300秒就不进行操作
			continue;
		}
		try {	// 太久未进行操作，为避免断开连接，主动向mysql发一个查询请求
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_last_oper_time = timeStamp;
			std::cout << "execute 'keep alive' query" << std::endl;
		}
		catch(sql::SQLException e){// 查询失败，重新创立一个连接替换现有连接
			std::cout << "execute query failed, error is: " << e.what() << std::endl;
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
			auto* newcon = driver->connect(url_, user_, pass_);
			newcon->setSchema(schema_);
			con->_con.reset(newcon);	// 将con中的_con智能指针重置为newcon
			con->_last_oper_time = timeStamp;
		}

	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this]() {	// 若条件变量中的函数返回false则释放锁等待唤醒，返回true则继续往下执行
		if (b_stop_) {
			return true;
		}
		return !pool_.empty();	// 若池子已空则释放锁等待其他线程归还连接
		});
	if (b_stop_) {
		return nullptr;
	}
	std::unique_ptr<SqlConnection> con = std::move(pool_.front());
	pool_.pop();
	return con;
}

void MysqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
	std::unique_lock<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;	// 池子关闭了也没有归还的必要了
	}
	pool_.push(std::move(con));
	cond_.notify_one();	// 唤醒一个正在等待的线程来拿连接
}

void MysqlPool::close() {
	b_stop_ = true;
	cond_.notify_all();
}

MysqlDao::MysqlDao() {
	auto& cfg = ConfigMgr::getInst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MysqlPool(host + ":" + port, user, pwd, schema, 5));
}
MysqlDao::~MysqlDao() {
	pool_->close();
}
int MysqlDao::regUser(const std::string name, const std::string email, const std::string passwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return -1;
		}
		// 准备调用存储过程（即mysql里的函数）
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置传入的三个参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, passwd);
		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

		// 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	    // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));	// 查询result的值获取输出结果
		if (res->next()) {
			int result = res->getInt("result");	// result为int类型，所以用getInt
			std::cout << "mysql query result: " << result << std::endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;	// -1表示操作失败
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
	
}
