#pragma once
// 作用类似于Qt里写的global.h
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "singleton.h"
#include <functional>
#include <map>;
#include <unordered_map>
#include <json/json.h>	// 包含json基本功能
#include <json/value.h>	// 一个json类型的结构（或者叫节点）
#include <json/reader.h>	// 把字符串解析成json value的结构

// 简化名称
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,
	RPCFailed = 1002,
};


