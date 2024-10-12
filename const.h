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

// 简化名称
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,	// Json解析错误
	RPCFailed = 1002,	// RPC请求错误
};


