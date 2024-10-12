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

class RPConPool {	// RPC���ӳ�
public:
	RPConPool(std::size_t poolSize, std::string host, std::string port);	// ��ʼ����Ա��������poolSize��stub��ӵ�������
	~RPConPool();
	void close();	// ����Ҫ��RPConPool��ȡ���ӵ��̣߳�˵��Ҫ�ر��ˣ���ɢ�˰�

	std::unique_ptr<VarifyService::Stub> getConnection();	// �ӳ�����ȡ������
	void returnConnection(std::unique_ptr<VarifyService::Stub> stub);	// ���������Ҫ�������ӹ黹�����ӳ�
private:
	std::atomic<bool> b_stop_;	// ��ʾ�Ƿ���Ҫ�����߳�(atomicȷ�������ֵ�ķ��ʺ��޸���ԭ�ӵģ��ṩ��һ����������ͬ�����ƣ��ڶ��̻߳����еļ򵥲������������)
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;	// �ö������������ӡ�STL���еĴ󲿷��������㷨�������̰߳�ȫ�ģ�Ҫ�û������Ȼ���ȷ���̰߳�ȫ
	std::mutex mutex_;	// ������
	std::condition_variable cond_;	// ��������
};

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>	// GRPC�Ŀͻ��ˣ����ڸ�grpc���������͡��������䡱������
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVarifyRsp getVerifyCode(std::string email);	// ��email����verify����ˣ���ȡGetVarify���͵Ļذ�
private:
	VerifyGrpcClient();	// ���캯����ʼ��һ��stub_
	//std::unique_ptr<VarifyService::Stub> stub_;	// ͨ����������ͨ�ţ��൱��һ��ý�顢��ʹ
	std::unique_ptr<RPConPool> pool_;	// �����ӳش���ԭ���ĵ���stub_����
};

