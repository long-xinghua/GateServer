#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

GetVarifyRsp VerifyGrpcClient::getVerifyCode(std::string email) {	// ��email����verify����ˣ���ȡGetVarify���͵Ļذ�
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	auto stub = pool_->getConnection();
	Status status = stub->GetVarifyCode(&context, request, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub));	// ������֮��黹����
		return reply;
	}
	else {
		pool_->returnConnection(std::move(stub));	// �����쳣ҲҪ�黹����
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VerifyGrpcClient::VerifyGrpcClient() {
	//std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	// stub_ = VarifyService::NewStub(channel);	//stub�����channelȥͨ�ţ�ʹ�ö��߳�ʱ����̷߳�����Ψһһ��stub������⣬����ҲҪ�������ӳأ�
	auto& cfgMgr = ConfigMgr::getInst();	// ��ȡ��������
	std::string host = cfgMgr["VarifyServer"]["Host"];
	std::string port = cfgMgr["VarifyServer"]["Port"];
	pool_.reset(new RPConPool(5, host, port));	//reset�ͷŵ�ǰ���󣬲�����ָ���¶��������ʼ���µ�RPConPool�������ӳ���ӵ��5������
}

RPConPool::RPConPool(std::size_t poolSize, std::string host, std::string port):
poolSize_(poolSize), host_(host), port_(port), b_stop_(false){
	for (size_t i = 0; i < poolSize_; i++) {	// ����size_��ͨ����ʹ�����grpc��������
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port, grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));	// ����VarifyService::NewStub(channel)��һ����ֵ��ʹ��pushʱ���õ����ƶ����죬����unique_ptr��Ҫ��
	}
}

RPConPool::~RPConPool() {
	std::lock_guard<std::mutex> lock(mutex_);	// �������Զ�������
	close();	// ���������߳���Ҫ�����ˣ�֮���ٹر�stub����
	while (!connections_.empty()) {
		connections_.pop();
	}
}

void RPConPool::close() {
	b_stop_ = true;
	cond_.notify_all();	// ֪ͨ�����̣߳�b_stop_��Ϊtrue
}

std::unique_ptr<VarifyService::Stub> RPConPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);	// ����,unique_lock��lock_guard���������ӳټ�������ʽ���������¼���
	cond_.wait(lock, [this]() {	// ��lambda���ʽ����falseʱ���ͷ������ȴ������������ѣ��������������
		if (b_stop_) {	// ����Ҫ�ر������ˣ��ȴ���ִ���꣨����ǮҪ�ص�Ҳ�ø��˶����ٹذɣ�
			return true;
		}
		return !connections_.empty();	// �����Ӷ���Ϊ�����ͷ���������ȴ����У�������ʱ����ȡ������
		});

	if (b_stop_) {
		return nullptr;	// ��ȡһ����ָ�룬��֪��һ�����ӹر���
	}
	// ������move��unique_ptr������Ȩ�����������򷵻ص���connections_.front()�����ã��ƻ���unique_ptr�Ĺ���
	auto stub = std::move(connections_.front());
	connections_.pop();	// move��ԭ���Ķ����ڣ�ֻ�Ǳ�Ϊ��nullptr������Ҫ����pop��ȥ
	return stub;	// ����stub�Ǹ��ֲ���������˷���ʱҲ�Ǹ���ֵ��ͨ���ƶ����彫unique_ptr������Ȩ�������÷�
}

void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub> stub) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {	// ���ӳ�Ҫ�ر��ˣ����ٸ����Ӽ��ɣ����ù黹��
		return;
	}

	connections_.push(std::move(stub));
	cond_.notify_one();	// ��������һ���������ˣ��������̷߳��źţ������߳��ڵȴ�ȡ�����ӾͿ���ȡ��
}