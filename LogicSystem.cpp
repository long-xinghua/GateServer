#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

void LogicSystem::regGet(std::string url, HttpHander handler) {
	_get_handlers.insert(make_pair(url, handler));	// ��url�ͻص�����д��map��
}

void LogicSystem::regPost(std::string url, HttpHander handler) {
	_post_handlers.insert(make_pair(url, handler));	// ��url�ͻص�����д��map��
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get test req" << std::endl;	//��_response.body()��д����Ϣ
		int i = 0;
		for (auto& elem : connection->_get_params) {	// ��ȡurl�еĲ����б�
			i++;
			beast::ostream(connection->_response.body()) << "param "<<i<<": "<<elem.first<<", value: "<<elem.second<<std::endl;
		}
		});

	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection) {	// ע���ȡ��֤���post����
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// ָ�����ͻ��˻�һ��json���͵�����
		Json::Value root;	// �����ͻ��˵�json����
		Json::Reader reader;	// ���ڽ�����Json����
		Json::Value src_root;	// �ͻ��˷�������json����
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse���ַ�����ʽ��json����body_str������Json::value���ͣ����ؽ���������ɹ���ʧ�ܣ�
		if (!parse_success) {	//	����ʧ��
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}

		if (!src_root.isMember("email")) {	// JSON�в�����"email"����
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}
		// û�����⣬ִ�к�������
		auto email = src_root["email"].asString();

		// ���˸��ͻ��˷�response����Ҫ��verify����˷�����
		GetVarifyRsp rsp =  VerifyGrpcClient::getInstance()->getVerifyCode(email);	// �����grpc�����������ӱ�����߳�ͬʱ���ʻ�����̰߳�ȫ���⣬�������ӳ������

		std::cout << "email is: " << email << std::endl;
		root["error"] = rsp.error();	// ���ص�error��rsp�е�error
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	regPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "receive body: " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");	// ָ�����ͻ��˻�һ��json���͵�����
		Json::Value root;	// �����ͻ��˵�json����
		Json::Reader reader;	// ���ڽ�����Json����
		Json::Value src_root;	// �ͻ��˷�������json����
		bool parse_success = reader.parse(body_str, src_root);	// Json::Reader::parse���ַ�����ʽ��json����body_str������Json::value���ͣ����ؽ���������ɹ���ʧ�ܣ�
		if (!parse_success) {	//	����ʧ��
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}
		if (!src_root.isMember("email")) {	// JSON�в�����"email"����
			std::cout << "no member called email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;	//�ذ�������һ�´�����Ϣ
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}

		std::string email = src_root["email"].asString();
		std::string user = src_root["user"].asString();
		std::string passwd = src_root["passwd"].asString();
		std::string confirm = src_root["confirm"].asString();
		std::string verifyCode = src_root["verifyCode"].asString();

		if (passwd != confirm) {	// ��ʹǰ��ȷ�Ϲ���������������Ƿ�һ�£������ﻹ��Ҫ�ж�һ�£�ȷ����ȫ
			root["error"] = ErrorCodes::PasswdErr;	//�ذ�������һ�´�����Ϣ
			std::cout << "passwd: " << passwd << " doesn't match confirm: " << confirm << std::endl;
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}

		// ����redis��������ȷ�Ͽͻ��˵���֤���Ƿ��redis�������е�һ��
		// ֮ǰconfig.ini�е�redis��������ַû�ģ�����һֱ����REDIS_ERR_PROTOCOL�����Ϊ����ip��ַ
		std::string trueVerifyCode;
		std::string key = CODEPREFIX + src_root["email"].asString();
		std::cout << "key:" << key << std::endl;
		bool b_get_varify_code = RedisMgr::getInstance()->get(key, trueVerifyCode);
		connection->_response.set(http::field::content_type, "text/json");	// ָ�����ͻ��˻�һ��json���͵�����
		if (!b_get_varify_code) {	// ��ȡredis�е���֤��ʧ��
			std::cout << "connect to redis failed or verifyCode is expired"<< trueVerifyCode << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;	// �ذ��д�����ϢΪ��֤�����
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}
		// ��ȡ��֤��ɹ����жϿͻ�����֤���redis��֤���Ƿ�һ��
		if (trueVerifyCode != src_root["verifyCode"].asString()) {
			std::cout << "wrong verify code!!" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;	// �ذ��д�����ϢΪ��֤�����
			std::string jsonstr = root.toStyledString();	// ��Json�ṹ���л�
			beast::ostream(connection->_response.body()) << jsonstr;	//�����л����json��Ϣ����_response�е�body
			return true;
		}

		// ��֤��һ��
		// ��ע����û���Ϣ��ӵ�mysql���ݿ���
		int uid = MysqlMgr::getInstance()->regUser(user, email, passwd);
		if (uid == 0 || uid == -1) {	// uidΪ0˵���û����������Ѿ������ˣ�-1˵��ִ��ʧ����
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::cout << "�û���Ϣע��ɹ�" << std::endl;

		root["error"] = ErrorCodes::Success;
		root["user"] = user;
		root["uid"] = uid;
		root["email"] = email;
		root["passwd"] = passwd;
		root["confirm"] = confirm;
		root["verifyCode"] = verifyCode;
		std::cout <<"���ͻ��˷��͵�uid��"<< root["uid"] << std::endl;  // ��� JSON �ַ����Լ������
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}

bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_get_handlers.find(path) == _get_handlers.end()) {	//�Ҳ���
		return false;	// ��HttpConnection���׳�404not found
	}

	_get_handlers[path](connection);	//�����ص���������
	return true;
}

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection> connection) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;	// ��HttpConnection���׳�404not found
	}
	_post_handlers[path](connection);	//�����ص���������
	return true;
}