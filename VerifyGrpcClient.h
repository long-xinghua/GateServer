#pragma once
// GateServer是verify服务器的客户端，通过grpc来发送请求
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"	// 通过grpc生成出来的头文件
#include "const.h"
#include "singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPConPool {	// RPC连接池
public:
	RPConPool(std::size_t poolSize, std::string host, std::string port);	// 初始化成员变量，将poolSize个stub添加到队列中
	~RPConPool();
	void close();	// 告诉要从RPConPool中取连接的线程，说我要关闭了，都散了吧

	std::unique_ptr<VarifyService::Stub> getConnection();	// 从池子中取出连接
	void returnConnection(std::unique_ptr<VarifyService::Stub> stub);	// 连接用完后要将该连接归还给连接池
private:
	std::atomic<bool> b_stop_;	// 表示是否需要回收线程(atomic确保对这个值的访问和修改是原子的，提供了一种轻量级的同步机制，在多线程环境中的简单操作可以用这个)
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;	// 用队列来保存连接。STL库中的大部分容器和算法都不是线程安全的，要用互斥锁等机制确保线程安全
	std::mutex mutex_;	// 互斥量
	std::condition_variable cond_;	// 条件变量
};

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>	// GRPC的客户端，用于给grpc服务器发送“发送邮箱”的请求
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVarifyRsp getVerifyCode(std::string email);	// 把email发给verify服务端，获取GetVarify类型的回包
private:
	VerifyGrpcClient();	// 构造函数初始化一下stub_
	//std::unique_ptr<VarifyService::Stub> stub_;	// 通过它跟别人通信，相当于一种媒介、信使
	std::unique_ptr<RPConPool> pool_;	// 用连接池代替原来的单个stub_连接
};

