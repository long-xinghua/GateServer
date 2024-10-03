#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket):_socket(std::move(socket)) {	// 由于socket没有默认构造，若不在初始化列表中对_socket进行初始化，则会自动调用其默认构造，
																				// 然而它没有默认构造，报错。它也没有拷贝构造，只能用移动构造
}
void HttpConnection::start() {
	auto self = shared_from_this();	// 防止回调之前自己被干掉
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {	//异步读取，函数立即返回，读取在后台进行，读取完成时才调用回调函数
		try {																									// _buffer中的数据被解析后会填充到_request
			if (ec) {	// 说明有错误(error_code重载了bool(),有错就为真)
				std::cout << "http read err is " << ec.what()<<std::endl;
				return;
			}
			// 没错误
			boost::ignore_unused(bytes_transfered);	//让编译器别弹警告，这是故意没使用变量(添加这个变量只是因为async_read参数中的回调函数必须要有这个参数)
			self->handleReq();
			self->checkDeadline();	// 检测超时
		}
		catch (std::exception& exp) {	//如果 try 块中的代码抛出任何继承自 std::exception 的异常，catch 块就会捕获并处理它
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}

unsigned char ToHex(unsigned char x)	//将十进制转为十六进制
{
	return  x > 9 ? x + 55 : x + 48;	
}

unsigned char FromHex(unsigned char x)	//十六进制转为十进制
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)	//	给url编码一下，统一成通用格式,如https://gitee.com/secondtonone1/llfcchat/blob/master/开发文档/day03-visualstudio配置boost和jsoncpp.md
{																			  //转成https://gitee.com/secondtonone1/llfcchat/blob/master/%E5%BC%80%E5%8F%91%E6%96%87%E6%A1%A3/day03-visualstudio%E9%85%8D%E7%BD%AEboost%E5%92%8Cjsoncpp.md
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符，转成加号拼上去
			strTemp += "+";
		else
		{
			//其他字符(如汉字)需要提前加%并且高四位和低四位分别转为16进制（一个汉字可能占多个字节）
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);	//将当前字节数据又移四位得到高四位，转成十六进制并拼到结果中
			strTemp += ToHex((unsigned char)str[i] & 0x0F);	//将当前字节与0x0F按位与得到第四位，转成十六进制并拼到结果中
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)	//url解码
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::preParseGetParam() {
	// 提取 URI  如 /get_test?key1=value1&key2=value2
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {	//没找到
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);	//问号前面的部分赋值给_get_url
	std::string query_string = uri.substr(query_pos + 1);	//参数列表从问号后一位一直到最后一位
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码（key和value可能是中文啥的，一堆百分号和16进制字符，需要先解码）  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;	//将键和值写入_get_params
		}
		query_string.erase(0, pos + 1);	//左闭右开区间，pos+1才能删掉“&”
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}


void HttpConnection::handleReq() {
	//设置版本
	_response.version(_request.version());
	_response.keep_alive(false);	//由于是http，设置短连接，告诉客户端服务器将在响应后关闭连接
	//http有多种请求，如get,set,put,post等,只处理其中几种
	if (_request.method() == http::verb::get) {	// 处理get请求
		preParseGetParam();	//在使用url之前先进行解析，得到参数前面的_get_url和参数键值对_get_params
		bool success = LogicSystem::getInstance()->handleGet(_get_url,shared_from_this());	//LogicSystem是单例类，用getInstance获取对象地址，来处理get请求
		if (!success) {	// 处理出错
			_response.result(http::status::not_found);	//枚举值，not_fount=404
			_response.set(http::field::content_type, "text/plain");	// 指定http头部为"text/plain",纯文本格式，帮助客户端理解、处理和显示相应内容（还有其他格式如图像类型、音视频类型、应用程序类型等）
			beast::ostream(_response.body()) << "url not found\r\n";	//往ostream里写就相当于往body里写了
			writeResponse();
			return;
		}
		_response.result(http::status::ok);	
		_response.set(http::field::server, "GateServer");	//告诉一下是哪个服务器给他回的response
		writeResponse();
	}

	if (_request.method() == http::verb::post) {	// 处理post请求, post请求无需解析链接中的参数
		bool success = LogicSystem::getInstance()->handlePost(_request.target(), shared_from_this());	//LogicSystem是单例类，用getInstance获取对象地址，来处理get请求
		if (!success) {	// 处理出错
			_response.result(http::status::not_found);	//枚举值，not_fount=404
			_response.set(http::field::content_type, "text/plain");	// 指定http头部为"text/plain",纯文本格式，帮助客户端理解、处理和显示相应内容（还有其他格式如图像类型、音视频类型、应用程序类型等）
			beast::ostream(_response.body()) << "url not found\r\n";	//往ostream里写就相当于往body里写了
			writeResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");	//告诉一下是哪个服务器给他回的response
		writeResponse();
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();	// 防止回调之前自己被干掉
	_response.content_length(_response.body().size());	// 设置一下回包长度，防止粘包
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);	//shutdown和close不一样，它只关闭一端的连接，发送端/接收端
		self->deadLine_.cancel();	// 取消掉定时器，这样就不会出发定时器超时的回调函数
		});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();
	deadLine_.async_wait([self](beast::error_code ec) {
		if (!ec) {	// 定时器没出错
			self->_socket.close(ec);	// 只有回报后长时间无响应才会调用这个回调函数，关闭客户端和服务端的连接（但服务器主动关闭连接会进入time_wait状态导致端口不可用！）
		}
		});
}