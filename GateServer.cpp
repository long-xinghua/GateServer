﻿// GateServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"

int main()
{
	try {
		unsigned short port = static_cast<unsigned short>(8080);	// 设置端口
		net::io_context ioc{ 1 };	// 底层设置一个线程来跑
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);	//指定捕获SIGINT(ctrl+C的信号)和SIGTERM(系统或其他进程发送的终止信号)信号
		signals.async_wait([&ioc](const boost::system::error_code &error, int signal_number) {	//触发信号调用回调函数
			if (error) {
				return;
			}
			ioc.stop();
			});

		std::make_shared<CServer>(ioc, port)->start();
		ioc.run();	//让事件轮询起来

	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}