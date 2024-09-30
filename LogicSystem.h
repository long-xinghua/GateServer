#pragma once
#include "const.h"

class HttpConnection;	//��ֹ������
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHander;

class LogicSystem:public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){}
	bool handleGet(std::string, std::shared_ptr<HttpConnection>);	// ����get����
	void regGet(std::string, HttpHander handler);	// ע��get����
private:
	LogicSystem();
	std::map<std::string, HttpHander> _post_handlers;	// ����post����Ļص���������
	std::map<std::string, HttpHander> _get_handlers;	// ����get����Ļص���������
};

