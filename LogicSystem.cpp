#include "LogicSystem.h"
#include "HttpConnection.h"

void LogicSystem::regGet(std::string url, HttpHander handler) {
	_get_handlers.insert(make_pair(url, handler));	// ��url�ͻص�����д��map��
}

void LogicSystem::regPost(std::string url, HttpHander handler) {
	_post_handlers.insert(make_pair(url, handler));	// ��url�ͻص�����д��map��
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get test req" << std::endl;	//��_response.body()��д����Ϣ
		int i = 0;
		for (auto& elem : connection->_get_params) {	// ��ȡurl�еĲ����б�
			i++;
			beast::ostream(connection->_response.body()) << "param "<<i<<": "<<elem.first<<", value: "<<elem.second<<std::endl;
		}
		});

	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection) {	// ע���ȡ��֤���post����
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// ָ�����ͻ��˻�һ��json���͵�����
		Json::Value root;	// �����ͻ��˵�json����
		Json::Reader reader;	// ���ڽ���Json����
		Json::Value src_root;	// �ͻ��˷�������json����
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse���ַ�����ʽ��json���ݽ�����Json::value���ͣ����ؽ���������ɹ���ʧ�ܣ�
		if (!parse_success) {	//	����ʧ��
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}

		if (!src_root.isMember("email")) {	// JSON�в�����"email"����
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}
		auto email = src_root["email"].asString();
		std::cout << "email is: " << email << std::endl;
		root["error"] = 0;
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

}

bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_get_handlers.find(path) == _get_handlers.end()) {	//�Ҳ���
		return false;	// ��HttpConnection���׳�404not found
	}

	_get_handlers[path](connection);	//�����ص���������
	return true;
}

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;	// ��HttpConnection���׳�404not found
	}
	_post_handlers[path](connection);	//�����ص���������
	return true;
}