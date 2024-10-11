#include "CServer.h"
#include "HttpConnection.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(),port)),
_socket(ioc){	//在初始化阶段将_acceptor和_socket绑定到ioc

}

void CServer::start() {
	auto self = shared_from_this();	//防止一些回调函数回调之前对象已经被析构的情况(异步函数可能在很久之后才调用)
	_acceptor.async_accept(_socket, [self](beast::error_code ec) {	// 开始监听连接请求，异步接收，连接情况储存在ec中
		try {
			//出错，放弃socket连接，继续监听其他连接
			if (ec) {
				self->start();
				return;
			}

			std::cout << "accept a new http connection" << std::endl;
			
			// 创建新连接，并创建HttpConnection类管理连接
			std::make_shared<HttpConnection>(std::move(self->_socket))->start();	//别直接定义一个局部变量，如HttpConnection(std::move(self->_socket))，这样随着作用域就释放掉了
																					//用智能指针能够更安全地管理HttpConnection对象的生命周期
			// 继续监听
			self->start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->start();
		}
		});
}
