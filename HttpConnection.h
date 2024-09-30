#pragma once
#include "const.h"
class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket socket);
	void start();	//开始监听读写事件
private:
	void checkDeadline();	// 检测超时的函数
	void writeResponse();	// 收到数据后进行应答
	void handleReq();	// 处理收到的请求
	tcp::socket _socket;
	beast::flat_buffer _buffer{8192};	//创建用于接收数据的缓冲区，大小为8K
	http::request<http::dynamic_body> _request;	//接收对方的请求，dynamic_body可以接受各种类型的请求，二进制的、图片的、表单的等
	http::response<http::dynamic_body> _response;	//回复对方请求
	net::steady_timer deadLine_{	//定义一个超时定时器
		_socket.get_executor(),std::chrono::seconds(60)	// boost的事件调度和定时器调度都通过底层事件轮询来做，要把它绑定到一个调度器
														// 在这里_socket已经在CServer中绑定到ioc了，用get_executor()就能获取这个调度器
	};

};

