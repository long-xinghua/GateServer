#pragma once
#include "const.h"
class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	//HttpConnection(tcp::socket socket);	// Http���Ӱ���CServer��socket��ֻ����һ���̣߳��󶨵�io_context���ӳ�������߲�������
	HttpConnection(boost::asio::io_context& ioc);	// io_contextû�п������죬Ҫ�����õķ�ʽ����
	void start();	//��ʼ������д�¼�
	tcp::socket& getSocket();	// Ҫ��HttpConnection�е�socket����accepterȥ�������ӣ�����Ҫ��getSocket()��socket����
private:
	void checkDeadline();	// ��ⳬʱ�ĺ���
	void writeResponse();	// �յ����ݺ����Ӧ��
	void handleReq();	// �����յ�������
	void preParseGetParam();	//���ڽ���url�еĲ��������䱣�浽_get_params��
	tcp::socket _socket;
	beast::flat_buffer _buffer{8192};	//�������ڽ������ݵĻ���������СΪ8K
	http::request<http::dynamic_body> _request;	//���նԷ�������dynamic_body���Խ��ܸ������͵����󣬶����Ƶġ�ͼƬ�ġ����ĵ�
	http::response<http::dynamic_body> _response;	//�ظ��Է�����
	net::steady_timer deadLine_{	//����һ����ʱ��ʱ��shi
		_socket.get_executor(),std::chrono::seconds(60)	// boost���¼����ȺͶ�ʱ�����ȶ�ͨ���ײ��¼���ѯ������Ҫ�����󶨵�һ��������
														// ������_socket�Ѿ���CServer�а󶨵�ioc�ˣ���get_executor()���ܻ�ȡ���������
	};
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

