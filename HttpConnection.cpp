#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket):_socket(std::move(socket)) {	// ����socketû��Ĭ�Ϲ��죬�����ڳ�ʼ���б��ж�_socket���г�ʼ��������Զ�������Ĭ�Ϲ��죬
																				// Ȼ����û��Ĭ�Ϲ��죬������Ҳû�п������죬ֻ�����ƶ�����
}
void HttpConnection::start() {
	auto self = shared_from_this();	// ��ֹ�ص�֮ǰ�Լ����ɵ�
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {	//�첽��ȡ�������������أ���ȡ�ں�̨���У���ȡ���ʱ�ŵ��ûص�����
		try {																									// _buffer�е����ݱ����������䵽_request
			if (ec) {	// ˵���д���(error_code������bool(),�д��Ϊ��)
				std::cout << "http read err is " << ec.what()<<std::endl;
				return;
			}
			// û����
			boost::ignore_unused(bytes_transfered);	//�ñ������𵯾��棬���ǹ���ûʹ�ñ���(����������ֻ����Ϊasync_read�����еĻص���������Ҫ���������)
			self->handleReq();
			self->checkDeadline();	// ��ⳬʱ
		}
		catch (std::exception& exp) {	//��� try ���еĴ����׳��κμ̳��� std::exception ���쳣��catch ��ͻᲶ�񲢴�����
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}

void HttpConnection::handleReq() {
	//���ð汾
	_response.version(_request.version());
	_response.keep_alive(false);	//������http�����ö����ӣ�����Ҫ��ʱ������
	//http�ж���������get,set,put,post��,ֻ�������м���
	if (_request.method() == http::verb::get) {	
		bool success = LogicSystem::getInstance()->handleGet(_request.target(),shared_from_this());	//LogicSystem�ǵ����࣬��getInstance��ȡ�����ַ��������get����
		if (!success) {	// �������
			_response.result(http::status::not_found);	//ö��ֵ��not_fount=404
			_response.set(http::field::content_type, "text/plain");	// ָ��httpͷ��Ϊ"text/plain"
			beast::ostream(_response.body()) << "url not found\r\n";	//��ostream��д���൱����body��д��
			writeResponse();
			return;
		}
		_response.result(http::status::ok);	
		_response.set(http::field::server, "GateServer");	//����һ�����ĸ������������ص�response
		writeResponse();
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();	// ��ֹ�ص�֮ǰ�Լ����ɵ�
	_response.content_length(_response.body().size());	// ����һ�»ذ����ȣ���ֹճ��
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);	//shutdown��close��һ������ֻ�ر�һ�˵����ӣ����Ͷ�/���ն�
		self->deadLine_.cancel();	// ȡ������ʱ���������Ͳ��������ʱ����ʱ�Ļص�����
		});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();
	deadLine_.async_wait([self](beast::error_code ec) {
		if (!ec) {	// ��ʱ��û����
			self->_socket.close(ec);	// ֻ�лر���ʱ������Ӧ�Ż��������ص��������رտͻ��˺ͷ���˵����ӣ��������������ر����ӻ����time_wait״̬���¶˿ڲ����ã���
		}
		});
}