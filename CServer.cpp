#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"	// ʹ�����ӳ�����������

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),	//�ڳ�ʼ���׶ν�_acceptor�󶨵�ioc
																	 _acceptor(ioc, tcp::endpoint(tcp::v4(),port)){		// ��ioc������ƽ���������
}

void CServer::start() {
	auto self = shared_from_this();	//��ֹһЩ�ص������ص�֮ǰ�����Ѿ������������(�첽���������ںܾ�֮��ŵ���)

	auto& io_context = AsioIOServicePool::getInstance()->GetIOService();	// ���һ�����ӳ��е������ģ����ڽ����������Ӱ󶨵���������
	std::shared_ptr<HttpConnection> new_connection = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_connection->getSocket(), [self, new_connection](beast::error_code ec) {	// ��ʼ�������������첽���գ��������������ec��
		try {
			//��������socket���ӣ�����������������
			if (ec) {
				self->start();
				return;
			}

			std::cout << "accept a new http connection" << std::endl;
			
			// ���������ӣ�������HttpConnection���������
			//std::make_shared<HttpConnection>(std::move(self->_socket))->start();	//��ֱ�Ӷ���һ���ֲ���������HttpConnection(std::move(self->_socket))������������������ͷŵ���
																					//������ָ���ܹ�����ȫ�ع���HttpConnection�������������
			//�������ӳغ������ڽ���֮ǰ�ʹ����ˣ�����ֻ��Ҫstart����
			new_connection->start();
																					
			// ��������
			self->start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->start();
		}
		});
}
