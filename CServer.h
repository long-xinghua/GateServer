#pragma once
#include "const.h"


class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);	//����һ�൱��epoll,�ײ���һ��ѭ�������¼������¼�����ʱ�׸��ϲ㴦��
	void start();

private:
	tcp::acceptor _acceptor;	// ������
	net::io_context& _ioc;	//Ҫ���������ͣ������ⲿ��������ʵ������û�п�������Ϳ�����ֵ
	tcp::socket _socket;
};

