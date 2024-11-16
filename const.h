#pragma once
// 作用类似于Qt里写的global.h
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>	// 包含json基本功能
#include <json/value.h>	// 一个json类型的结构（或者叫节点）
#include <json/reader.h>	// 把字符串解析成json value的结构
#include <boost/filesystem.hpp>	// 用boost库里的文件系统来管理文件，在windows和linux中都能用
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hiredis.h"
#include <cassert>


// 简化名称
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,		// Json解析错误
	RPCFailed = 1002,		// RPC请求错误
	VerifyExpired = 1003,	// 验证码过期
	VerifyCodeErr = 1004,	// 验证码错误
	UserExist = 1005,		// 用户已存在
	PasswdErr = 1006,		// 密码错误
	EmailNoMatch = 1007,	// 邮箱不匹配(用户名或者邮箱错误)
	PasswdUpdateErr = 1008,	// 更新密码失败
	PasswdInvalid = 1009,	// 密码不符规格
	RPCGetFailed = 1010,	// 获取RPC请求失败
};

#define CODEPREFIX  "code_"


// Defer类用于实现类似go语言中的defer功能，在Defer对象作用域结束之前一定会调用一次func_函数。
class Defer {
public:
	// 构造函数接收一个函数指针或lambda表达式
	Defer(std::function<void()> func):func_(func){}
	// 在调出其作用域时调用析构函数，析构函数中调用待执行的func_函数
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};
