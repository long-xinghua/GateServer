#pragma once
// GateServer��verify�������Ŀͻ��ˣ�ͨ��grpc����������
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"	// ͨ��grpc���ɳ�����ͷ�ļ�
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
	GetVarifyRsp getVerifyCode(std::string email) {	// ��email����verify����ˣ���ȡGetVarify���͵Ļذ�
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
		stub_ = VarifyService::NewStub(channel);	//stub�����channelȥͨ��
	}
	std::unique_ptr<VarifyService::Stub> stub_;	// ͨ����������ͨ�ţ��൱��һ��ý��
};

