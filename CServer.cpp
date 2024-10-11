#include "CServer.h"
#include "HttpConnection.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(),port)),
_socket(ioc){	//�ڳ�ʼ���׶ν�_acceptor��_socket�󶨵�ioc

}

void CServer::start() {
	auto self = shared_from_this();	//��ֹһЩ�ص������ص�֮ǰ�����Ѿ������������(�첽���������ںܾ�֮��ŵ���)
	_acceptor.async_accept(_socket, [self](beast::error_code ec) {	// ��ʼ�������������첽���գ��������������ec��
		try {
			//��������socket���ӣ�����������������
			if (ec) {
				self->start();
				return;
			}

			std::cout << "accept a new http connection" << std::endl;
			
			// ���������ӣ�������HttpConnection���������
			std::make_shared<HttpConnection>(std::move(self->_socket))->start();	//��ֱ�Ӷ���һ���ֲ���������HttpConnection(std::move(self->_socket))������������������ͷŵ���
																					//������ָ���ܹ�����ȫ�ع���HttpConnection�������������
			// ��������
			self->start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->start();
		}
		});
}
