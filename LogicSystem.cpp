#include "LogicSystem.h"
#include "HttpConnection.h"

void LogicSystem::regGet(std::string url, HttpHander handler) {
	_get_handlers.insert(make_pair(url, handler));	// 将url和回调函数写入map中
}

void LogicSystem::regPost(std::string url, HttpHander handler) {
	_post_handlers.insert(make_pair(url, handler));	// 将url和回调函数写入map中
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get test req" << std::endl;	//往_response.body()中写入信息
		int i = 0;
		for (auto& elem : connection->_get_params) {	// 获取url中的参数列表
			i++;
			beast::ostream(connection->_response.body()) << "param "<<i<<": "<<elem.first<<", value: "<<elem.second<<std::endl;
		}
		});

	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection) {	// 注册获取验证码的post请求
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// 将客户端发过来的数据转换成字符串类型
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// 指定给客户端回一个json类型的数据
		Json::Value root;	// 发给客户端的json数据
		Json::Reader reader;	// 用于解析Json数据
		Json::Value src_root;	// 客户端发过来的json数据
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse将字符串格式的json数据解析成Json::value类型，返回解析情况（成功或失败）
		if (!parse_success) {	//	解析失败
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
			return true;
		}

		if (!src_root.isMember("email")) {	// JSON中不存在"email"的项
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//回包中设置一下错误信息
			std::string jsonstr = root.toStyledString();	// 将Json结构序列化
			beast::ostream(connection->_response.body()) << jsonstr;	//将序列化后的json信息给到_response中的body
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
	if (_get_handlers.find(path) == _get_handlers.end()) {	//找不到
		return false;	// 让HttpConnection中抛出404not found
	}

	_get_handlers[path](connection);	//交给回调函数处理
	return true;
}

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;	// 让HttpConnection中抛出404not found
	}
	_post_handlers[path](connection);	//交给回调函数处理
	return true;
}