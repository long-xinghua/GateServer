#include "LogicSystem.h"
#include "HttpConnection.h"

void LogicSystem::regGet(std::string url, HttpHander handler) {
	_get_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get test req";
		});
}

bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_get_handlers.find(path) == _get_handlers.end()) {	//�Ҳ���
		return false;	// ��HttpConnection���׳�404not found
	}

	_get_handlers[path](connection);	//�����ص���������
	return true;
}