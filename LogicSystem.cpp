#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

void LogicSystem::regGet(std::string url, HttpHander handler) {
	_get_handlers.insert(make_pair(url, handler));	// 将url和回调函数写入map中
}

void LogicSystem::regPost(std::string url, HttpHander handler) {
	_post_handlers.insert(make_pair(url, handler));	// 将url和回调函数写入map中
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get test req" << std::endl;	//往_response.body()中写入信息
		int i = 0;
		for (auto& elem : connection->_get_params) {	// 获取url中的参数列表
			i++;
			beast::ostream(connection->_response.body()) << "param "<<i<<": "<<elem.first<<", value: "<<elem.second<<std::endl;
		}
		});

	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection) {	// 注册获取验证码的post请求
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// 将客户端发过来的数据转换成字符串类型
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// 指定给客户端回一个json类型的数据
		Json::Value root;	// 发给客户端的json数据
		Json::Reader reader;	// 用于解析出Json数据
		Json::Value src_root;	// 客户端发过来的json数据
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse将字符串格式的json数据body_str解析成Json::value类型，返回解析情况（成功或失败）
		if (!parse_success) {	//	解析失败
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}

		if (!src_root.isMember("email")) {	// JSON中不存在"email"的项
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}
		// 没有问题，执行后续操作
		auto email = src_root["email"].asString();

		// 除了给客户端发response，还要给verify服务端发请求
		GetVarifyRsp rsp =  VerifyGrpcClient::getInstance()->getVerifyCode(email);	// 如果与grpc服务器的连接被多个线程同时访问会出现线程安全问题，可用连接池来解决

		std::cout << "email is: " << email << std::endl;
		root["error"] = rsp.error();	// 返回的error是rsp中的error
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	regPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// 将客户端发过来的数据转换成字符串类型
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// 指定给客户端回一个json类型的数据
		Json::Value root;	// 发给客户端的json数据
		Json::Reader reader;	// 用于解析出Json数据
		Json::Value src_root;	// 客户端发过来的json数据
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse将字符串格式的json数据body_str解析成Json::value类型，返回解析情况（成功或失败）
		if (!parse_success) {	//	解析失败
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}
		if (!src_root.isMember("email")) {	// JSON中不存在"email"的项
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}

		std::string email = src_root["email"].asString();
		std::string user = src_root["user"].asString();
		std::string passwd = src_root["passwd"].asString();
		std::string confirm = src_root["confirm"].asString();
		std::string verifyCode = src_root["verifyCode"].asString();

		if (passwd != confirm) {	// 即使前端确认过两次输入的密码是否一致，在这里还是要判断一下，确保安全
			root["error"] = ErrorCodes::PasswdErr;	//回包中设置一下错误信息
			std::cout << "passwd: " << passwd << " doesn't match confirm: " << confirm << std::endl;
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}

		// 连接redis服务器，确认客户端的验证码是否和redis服务器中的一致
		// 之前config.ini中的redis服务器地址没改，导致一直返回REDIS_ERR_PROTOCOL，后改为本地ip地址
		std::string trueVerifyCode;
		std::string key = CODEPREFIX + src_root["email"].asString();
		std::cout << "key:" << key << std::endl;
		bool b_get_varify_code = RedisMgr::getInstance()->get(key, trueVerifyCode);
		connection->_response.set(http::field::content_type, "text/json");	// 指定给客户端回一个json类型的数据
		if (!b_get_varify_code) {	// 获取redis中的验证码失败
			std::cout << "connect to redis failed or verifyCode is expired"<< trueVerifyCode << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;	// 回包中错误信息为验证码过期
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}
		// 获取验证码成功，判断客户端验证码和redis验证码是否一致
		if (trueVerifyCode != src_root["verifyCode"].asString()) {
			std::cout << "wrong verify code!!" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;	// 回包中错误信息为验证码错误
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}

		// 验证码一致
		// 将注册的用户信息添加到mysql数据库中
		int uid = MysqlMgr::getInstance()->regUser(user, email, passwd);
		if (uid == 0 || uid == -1) {	// uid为0说明用户名或邮箱已经存在了，-1说明执行失败了
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::cout << "用户信息注册成功" << std::endl;

		root["error"] = ErrorCodes::Success;
		root["user"] = user;
		root["uid"] = uid;
		root["email"] = email;
		root["passwd"] = passwd;
		root["confirm"] = confirm;
		root["verifyCode"] = verifyCode;
		std::cout <<"给客户端发送的uid："<< root["uid"] << std::endl;  // 输出 JSON 字符串以检查内容
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}

bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_get_handlers.find(path) == _get_handlers.end()) {	//找不到
		return false;	// 让HttpConnection中抛出404not found
	}

	_get_handlers[path](connection);	//交给回调函数处理
	return true;
}

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;	// 让HttpConnection中抛出404not found
	}
	_post_handlers[path](connection);	//交给回调函数处理
	return true;
}