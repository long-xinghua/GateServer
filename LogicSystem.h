#pragma once
#include "const.h"

class HttpConnection;	//防止互引用
using HttpHander = std::function<void(std::shared_ptr<HttpConnection>)>;	//	给函数对象取个名字方便使用

class LogicSystem:public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){}
	bool handleGet(std::string, std::shared_ptr<HttpConnection>);	// 处理get请求
	void regGet(std::string, HttpHander handler);	// 注册get请求

	bool handlePost(std::string, std::shared_ptr<HttpConnection>);	// 处理post请求
	void regPost(std::string, HttpHander handler);	// 注册post请求
private:
	LogicSystem();
	std::map<std::string, HttpHander> _post_handlers;	// 处理post请求的回调函数集合
	std::map<std::string, HttpHander> _get_handlers;	// 处理get请求的回调函数集合
};

