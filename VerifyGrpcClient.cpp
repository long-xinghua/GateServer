#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

GetVarifyRsp VerifyGrpcClient::getVerifyCode(std::string email) {	// 把email发给verify服务端，获取GetVarify类型的回包
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	auto stub = pool_->getConnection();
	Status status = stub->GetVarifyCode(&context, request, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub));	// 用完了之后归还连接
		return reply;
	}
	else {
		pool_->returnConnection(std::move(stub));	// 出现异常也要归还连接
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VerifyGrpcClient::VerifyGrpcClient() {
	//std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	// stub_ = VarifyService::NewStub(channel);	//stub用这个channel去通信（使用多线程时多个线程访问这唯一一个stub会出问题，所以也要用上连接池）
	auto& cfgMgr = ConfigMgr::getInst();	// 获取配置数据
	std::string host = cfgMgr["VarifyServer"]["Host"];
	std::string port = cfgMgr["VarifyServer"]["Port"];
	pool_.reset(new RPConPool(5, host, port));	//reset释放当前对象，并可以指向新对象，这里初始化新的RPConPool对象，连接池中拥有5个连接
}

RPConPool::RPConPool(std::size_t poolSize, std::string host, std::string port):
poolSize_(poolSize), host_(host), port_(port), b_stop_(false){
	for (size_t i = 0; i < poolSize_; i++) {	// 创建size_个通信信使，提高grpc并发能力
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port, grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));	// 由于VarifyService::NewStub(channel)是一个右值，使用push时调用的是移动构造，符合unique_ptr的要求
	}
}

RPConPool::~RPConPool() {
	std::lock_guard<std::mutex> lock(mutex_);	// 加锁（自动解锁）
	close();	// 告诉所有线程我要关门了，之后再关闭stub连接
	while (!connections_.empty()) {
		connections_.pop();
	}
}

void RPConPool::close() {
	b_stop_ = true;
	cond_.notify_all();	// 通知各个线程，b_stop_已为true
}

std::unique_ptr<VarifyService::Stub> RPConPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);	// 加锁,unique_lock比lock_guard更灵活，允许延迟加锁、显式解锁和重新加锁
	cond_.wait(lock, [this]() {	// 该lambda表达式返回false时会释放锁，等待条件变量唤醒，否则继续往下走
		if (b_stop_) {	// 表明要关闭连接了，先带锁执行完（交了钱要关店也得给了东西再关吧）
			return true;
		}
		return !connections_.empty();	// 若连接队列为空则释放锁，进入等待队列，有连接时才能取出连接
		});

	if (b_stop_) {
		return nullptr;	// 获取一个空指针，告知上一级连接关闭了
	}
	// 必须用move将unique_ptr的所有权交出来，否则返回的是connections_.front()的引用，破坏了unique_ptr的规则
	auto stub = std::move(connections_.front());
	connections_.pop();	// move后原来的对象还在，只是变为了nullptr，所以要将其pop出去
	return stub;	// 由于stub是个局部变量，因此返回时也是个右值，通过移动语义将unique_ptr的所有权交给调用方
}

void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub> stub) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {	// 连接池要关闭了，销毁该连接即可，不用归还了
		return;
	}

	connections_.push(std::move(stub));
	cond_.notify_one();	// 池子里有一个新连接了，给单个线程发信号，若有线程在等待取出连接就可以取了
}