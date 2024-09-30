#pragma once
#include "const.h"


class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);	//参数一相当于epoll,底层跑一个循环监听事件，有事件发生时抛给上层处理
	void start();

private:
	tcp::acceptor _acceptor;	// 接收器
	net::io_context& _ioc;	//要用引用类型，接受外部传过来的实例，它没有拷贝构造和拷贝赋值
	tcp::socket _socket;
};

