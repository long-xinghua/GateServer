#pragma once
#include "const.h"

class HttpConnection;	//防止互引用
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHander;

class LogicSystem:public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){}
	bool handleGet(std::string, std::shared_ptr<HttpConnection>);	// 处理get请求
	void regGet(std::string, HttpHander handler);	// 注册get请求
private:
	LogicSystem();
	std::map<std::string, HttpHander> _post_handlers;	// 处理post请求的回调函数集合
	std::map<std::string, HttpHander> _get_handlers;	// 处理get请求的回调函数集合
};

