#pragma once
// ����������Qt��д��global.h
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>	// ����json��������
#include <json/value.h>	// һ��json���͵Ľṹ�����߽нڵ㣩
#include <json/reader.h>	// ���ַ���������json value�Ľṹ
#include <boost/filesystem.hpp>	// ��boost������ļ�ϵͳ�������ļ�����windows��linux�ж�����
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hiredis.h"
#include <cassert>


// ������
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,		// Json��������
	RPCFailed = 1002,		// RPC�������
	VerifyExpired = 1003,	// ��֤�����
	VerifyCodeErr = 1004,	// ��֤�����
	UserExist = 1005,		// �û��Ѵ���
	PasswdErr = 1006,		// �������
	EmailNoMatch = 1007,	// ���䲻ƥ��(�û��������������)
	PasswdUpdateErr = 1008,	// ��������ʧ��
	PasswdInvalid = 1009,	// ���벻�����
	RPCGetFailed = 1010,	// ��ȡRPC����ʧ��
};

#define CODEPREFIX  "code_"


// Defer������ʵ������go�����е�defer���ܣ���Defer�������������֮ǰһ�������һ��func_������
class Defer {
public:
	// ���캯������һ������ָ���lambda���ʽ
	Defer(std::function<void()> func):func_(func){}
	// �ڵ�����������ʱ�����������������������е��ô�ִ�е�func_����
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};
