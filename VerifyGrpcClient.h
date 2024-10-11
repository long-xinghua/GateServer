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

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVarifyRsp getVerifyCode(std::string email) {	// 把email发给verify服务端，获取GetVarify类型的回包
		ClientContext context;
		GetVarifyRsp reply;
		GetVarifyReq request;
		request.set_email(email);
		Status status = stub_->GetVarifyCode(&context, request, &reply);
		if (status.ok()) {
			return reply;
		}
		else {
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}	
private:
	VerifyGrpcClient() {
		std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
		stub_ = VarifyService::NewStub(channel);	//stub用这个channel去通信
	}
	std::unique_ptr<VarifyService::Stub> stub_;	// 通过它跟别人通信，相当于一种媒介
};

