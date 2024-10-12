#pragma once
#include "const.h"
class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	//HttpConnection(tcp::socket socket);	// Http连接绑定在CServer的socket上只用了一个线程，绑定到io_context连接池中能提高并发能力
	HttpConnection(boost::asio::io_context& ioc);	// io_context没有拷贝构造，要用引用的方式传递
	void start();	//开始监听读写事件
	tcp::socket& getSocket();	// 要将HttpConnection中的socket交给accepter去接收连接，所以要用getSocket()把socket给它
private:
	void checkDeadline();	// 检测超时的函数
	void writeResponse();	// 收到数据后进行应答
	void handleReq();	// 处理收到的请求
	void preParseGetParam();	//用于解析url中的参数并将其保存到_get_params中
	tcp::socket _socket;
	beast::flat_buffer _buffer{8192};	//创建用于接收数据的缓冲区，大小为8K
	http::request<http::dynamic_body> _request;	//接收对方的请求，dynamic_body可以接受各种类型的请求，二进制的、图片的、表单的等
	http::response<http::dynamic_body> _response;	//回复对方请求
	net::steady_timer deadLine_{	//定义一个超时定时器shi
		_socket.get_executor(),std::chrono::seconds(60)	// boost的事件调度和定时器调度都通过底层事件轮询来做，要把它绑定到一个调度器
														// 在这里_socket已经在CServer中绑定到ioc了，用get_executor()就能获取这个调度器
	};
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

