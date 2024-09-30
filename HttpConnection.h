#pragma once
#include "const.h"
class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket socket);
	void start();	//��ʼ������д�¼�
private:
	void checkDeadline();	// ��ⳬʱ�ĺ���
	void writeResponse();	// �յ����ݺ����Ӧ��
	void handleReq();	// �����յ�������
	tcp::socket _socket;
	beast::flat_buffer _buffer{8192};	//�������ڽ������ݵĻ���������СΪ8K
	http::request<http::dynamic_body> _request;	//���նԷ�������dynamic_body���Խ��ܸ������͵����󣬶����Ƶġ�ͼƬ�ġ����ĵ�
	http::response<http::dynamic_body> _response;	//�ظ��Է�����
	net::steady_timer deadLine_{	//����һ����ʱ��ʱ��
		_socket.get_executor(),std::chrono::seconds(60)	// boost���¼����ȺͶ�ʱ�����ȶ�ͨ���ײ��¼���ѯ������Ҫ�����󶨵�һ��������
														// ������_socket�Ѿ���CServer�а󶨵�ioc�ˣ���get_executor()���ܻ�ȡ���������
	};

};

