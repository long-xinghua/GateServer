#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket):_socket(std::move(socket)) {	// ����socketû��Ĭ�Ϲ��죬�����ڳ�ʼ���б��ж�_socket���г�ʼ��������Զ�������Ĭ�Ϲ��죬
																				// Ȼ����û��Ĭ�Ϲ��죬������Ҳû�п������죬ֻ�����ƶ�����
}
void HttpConnection::start() {
	auto self = shared_from_this();	// ��ֹ�ص�֮ǰ�Լ����ɵ�
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {	//�첽��ȡ�������������أ���ȡ�ں�̨���У���ȡ���ʱ�ŵ��ûص�����
		try {																									// _buffer�е����ݱ����������䵽_request
			if (ec) {	// ˵���д���(error_code������bool(),�д��Ϊ��)
				std::cout << "http read err is " << ec.what()<<std::endl;
				return;
			}
			// û����
			boost::ignore_unused(bytes_transfered);	//�ñ������𵯾��棬���ǹ���ûʹ�ñ���(����������ֻ����Ϊasync_read�����еĻص���������Ҫ���������)
			self->handleReq();
			self->checkDeadline();	// ��ⳬʱ
		}
		catch (std::exception& exp) {	//��� try ���еĴ����׳��κμ̳��� std::exception ���쳣��catch ��ͻᲶ�񲢴�����
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}

unsigned char ToHex(unsigned char x)	//��ʮ����תΪʮ������
{
	return  x > 9 ? x + 55 : x + 48;	
}

unsigned char FromHex(unsigned char x)	//ʮ������תΪʮ����
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)	//	��url����һ�£�ͳһ��ͨ�ø�ʽ,��https://gitee.com/secondtonone1/llfcchat/blob/master/�����ĵ�/day03-visualstudio����boost��jsoncpp.md
{																			  //ת��https://gitee.com/secondtonone1/llfcchat/blob/master/%E5%BC%80%E5%8F%91%E6%96%87%E6%A1%A3/day03-visualstudio%E9%85%8D%E7%BD%AEboost%E5%92%8Cjsoncpp.md
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//�ж��Ƿ�������ֺ���ĸ����
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //Ϊ���ַ���ת�ɼӺ�ƴ��ȥ
			strTemp += "+";
		else
		{
			//�����ַ�(�纺��)��Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16���ƣ�һ�����ֿ���ռ����ֽڣ�
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);	//����ǰ�ֽ�����������λ�õ�����λ��ת��ʮ�����Ʋ�ƴ�������
			strTemp += ToHex((unsigned char)str[i] & 0x0F);	//����ǰ�ֽ���0x0F��λ��õ�����λ��ת��ʮ�����Ʋ�ƴ�������
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)	//url����
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//��ԭ+Ϊ��
		if (str[i] == '+') strTemp += ' ';
		//����%������������ַ���16����תΪchar��ƴ��
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
	// ��ȡ URI  �� /get_test?key1=value1&key2=value2
	auto uri = _request.target();
	// ���Ҳ�ѯ�ַ����Ŀ�ʼλ�ã��� '?' ��λ�ã�  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {	//û�ҵ�
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);	//�ʺ�ǰ��Ĳ��ָ�ֵ��_get_url
	std::string query_string = uri.substr(query_pos + 1);	//�����б���ʺź�һλһֱ�����һλ
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // ������ url_decode ����������URL���루key��value����������ɶ�ģ�һ�Ѱٷֺź�16�����ַ�����Ҫ�Ƚ��룩  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;	//������ֵд��_get_params
		}
		query_string.erase(0, pos + 1);	//����ҿ����䣬pos+1����ɾ����&��
	}
	// �������һ�������ԣ����û�� & �ָ�����  
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
	//���ð汾
	_response.version(_request.version());
	_response.keep_alive(false);	//������http�����ö����ӣ����߿ͻ��˷�����������Ӧ��ر�����
	//http�ж���������get,set,put,post��,ֻ�������м���
	if (_request.method() == http::verb::get) {	// ����get����
		preParseGetParam();	//��ʹ��url֮ǰ�Ƚ��н������õ�����ǰ���_get_url�Ͳ�����ֵ��_get_params
		bool success = LogicSystem::getInstance()->handleGet(_get_url,shared_from_this());	//LogicSystem�ǵ����࣬��getInstance��ȡ�����ַ��������get����
		if (!success) {	// �������
			_response.result(http::status::not_found);	//ö��ֵ��not_fount=404
			_response.set(http::field::content_type, "text/plain");	// ָ��httpͷ��Ϊ"text/plain",���ı���ʽ�������ͻ�����⡢�������ʾ��Ӧ���ݣ�����������ʽ��ͼ�����͡�����Ƶ���͡�Ӧ�ó������͵ȣ�
			beast::ostream(_response.body()) << "url not found\r\n";	//��ostream��д���൱����body��д��
			writeResponse();
			return;
		}
		_response.result(http::status::ok);	
		_response.set(http::field::server, "GateServer");	//����һ�����ĸ������������ص�response
		writeResponse();
	}

	if (_request.method() == http::verb::post) {	// ����post����, post����������������еĲ���
		bool success = LogicSystem::getInstance()->handlePost(_request.target(), shared_from_this());	//LogicSystem�ǵ����࣬��getInstance��ȡ�����ַ��������get����
		if (!success) {	// �������
			_response.result(http::status::not_found);	//ö��ֵ��not_fount=404
			_response.set(http::field::content_type, "text/plain");	// ָ��httpͷ��Ϊ"text/plain",���ı���ʽ�������ͻ�����⡢�������ʾ��Ӧ���ݣ�����������ʽ��ͼ�����͡�����Ƶ���͡�Ӧ�ó������͵ȣ�
			beast::ostream(_response.body()) << "url not found\r\n";	//��ostream��д���൱����body��д��
			writeResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");	//����һ�����ĸ������������ص�response
		writeResponse();
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();	// ��ֹ�ص�֮ǰ�Լ����ɵ�
	_response.content_length(_response.body().size());	// ����һ�»ذ����ȣ���ֹճ��
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);	//shutdown��close��һ������ֻ�ر�һ�˵����ӣ����Ͷ�/���ն�
		self->deadLine_.cancel();	// ȡ������ʱ���������Ͳ��������ʱ����ʱ�Ļص�����
		});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();
	deadLine_.async_wait([self](beast::error_code ec) {
		if (!ec) {	// ��ʱ��û����
			self->_socket.close(ec);	// ֻ�лر���ʱ������Ӧ�Ż��������ص��������رտͻ��˺ͷ���˵����ӣ��������������ر����ӻ����time_wait״̬���¶˿ڲ����ã���
		}
		});
}