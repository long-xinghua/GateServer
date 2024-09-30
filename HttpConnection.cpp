#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket):_socket(std::move(socket)) {	// 由于socket没有默认构造，若不在初始化列表中对_socket进行初始化，则会自动调用其默认构造，
																				// 然而它没有默认构造，报错。它也没有拷贝构造，只能用移动构造
}
void HttpConnection::start() {
	auto self = shared_from_this();	// 防止回调之前自己被干掉
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {	//异步读取，函数立即返回，读取在后台进行，读取完成时才调用回调函数
		try {																									// _buffer中的数据被解析后会填充到_request
			if (ec) {	// 说明有错误(error_code重载了bool(),有错就为真)
				std::cout << "http read err is " << ec.what()<<std::endl;
				return;
			}
			// 没错误
			boost::ignore_unused(bytes_transfered);	//让编译器别弹警告，这是故意没使用变量(添加这个变量只是因为async_read参数中的回调函数必须要有这个参数)
			self->handleReq();
			self->checkDeadline();	// 检测超时
		}
		catch (std::exception& exp) {	//如果 try 块中的代码抛出任何继承自 std::exception 的异常，catch 块就会捕获并处理它
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}

void HttpConnection::handleReq() {
	//设置版本
	_response.version(_request.version());
	_response.keep_alive(false);	//由于是http，设置短连接，不需要长时间连接
	//http有多种请求，如get,set,put,post等,只处理其中几种
	if (_request.method() == http::verb::get) {	
		bool success = LogicSystem::getInstance()->handleGet(_request.target(),shared_from_this());	//LogicSystem是单例类，用getInstance获取对象地址，来处理get请求
		if (!success) {	// 处理出错
			_response.result(http::status::not_found);	//枚举值，not_fount=404
			_response.set(http::field::content_type, "text/plain");	// 指定http头部为"text/plain"
			beast::ostream(_response.body()) << "url not found\r\n";	//往ostream里写就相当于往body里写了
			writeResponse();
			return;
		}
		_response.result(http::status::ok);	
		_response.set(http::field::server, "GateServer");	//告诉一下是哪个服务器给他回的response
		writeResponse();
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();	// 防止回调之前自己被干掉
	_response.content_length(_response.body().size());	// 设置一下回包长度，防止粘包
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);	//shutdown和close不一样，它只关闭一端的连接，发送端/接收端
		self->deadLine_.cancel();	// 取消掉定时器，这样就不会出发定时器超时的回调函数
		});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();
	deadLine_.async_wait([self](beast::error_code ec) {
		if (!ec) {	// 定时器没出错
			self->_socket.close(ec);	// 只有回报后长时间无响应才会调用这个回调函数，关闭客户端和服务端的连接（但服务器主动关闭连接会进入time_wait状态导致端口不可用！）
		}
		});
}