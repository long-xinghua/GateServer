#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"	// 使用连接池来分配连接

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),	//在初始化阶段将_acceptor绑定到ioc
																	 _acceptor(ioc, tcp::endpoint(tcp::v4(),port)){		// 该ioc负责控制接收新连接
}

void CServer::start() {
	auto self = shared_from_this();	//防止一些回调函数回调之前对象已经被析构的情况(异步函数可能在很久之后才调用)

	auto& io_context = AsioIOServicePool::getInstance()->GetIOService();	// 获得一个连接池中的上下文，用于将创建的连接绑定到该上下文
	std::shared_ptr<HttpConnection> new_connection = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_connection->getSocket(), [self, new_connection](beast::error_code ec) {	// 开始监听连接请求，异步接收，连接情况储存在ec中
		try {
			//出错，放弃socket连接，继续监听其他连接
			if (ec) {
				self->start();
				return;
			}

			std::cout << "accept a new http connection" << std::endl;
			
			// 创建新连接，并创建HttpConnection类管理连接
			//std::make_shared<HttpConnection>(std::move(self->_socket))->start();	//别直接定义一个局部变量，如HttpConnection(std::move(self->_socket))，这样随着作用域就释放掉了
																					//用智能指针能够更安全地管理HttpConnection对象的生命周期
			//引入连接池后连接在接收之前就创建了，在这只需要start即可
			new_connection->start();
																					
			// 继续监听
			self->start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->start();
		}
		});
}
