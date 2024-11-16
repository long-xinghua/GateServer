#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"

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

	// ע���ȡ��֤���post����
	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection) {	
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "request: get_varifyCode" << std::endl;
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

	// ע���û�������
	regPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {	
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "request: user_register" << std::endl;
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

	// ������������
	regPost("/reset_passwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "request: reset_passwd" << std::endl;
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
		std::string verifyCode = src_root["verifyCode"].asString();


		// ����redis��������ȷ�Ͽͻ��˵���֤���Ƿ��redis�������е�һ��
		// ֮ǰconfig.ini�е�redis��������ַû�ģ�����һֱ����REDIS_ERR_PROTOCOL�����Ϊ����ip��ַ
		std::string trueVerifyCode;
		std::string key = CODEPREFIX + email;
		std::cout << "key:" << key << std::endl;
		bool b_get_varify_code = RedisMgr::getInstance()->get(key, trueVerifyCode);
		connection->_response.set(http::field::content_type, "text/json");	// ָ�����ͻ��˻�һ��json���͵�����
		if (!b_get_varify_code) {	// ��ȡredis�е���֤��ʧ��
			std::cout << "connect to redis failed or verifyCode is expired" << trueVerifyCode << std::endl;
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

		// ��ѯ�û����������Ƿ�ƥ��(����о�Ӧ��������ȥ��֤�û�������Ϊ������Ψһ�ģ��û�������Ψһ�����û����������ݿ��е�������ܵõ��ü����������Ҫ���˽��)
		bool valid = MysqlMgr::getInstance()->checkEmail(user, email);
		if (!valid) {	// �û������������
			std::cout << "user not exists or user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNoMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//�û���������ƥ�䣬����Mysql�е�����
		bool b_update = MysqlMgr::getInstance()->updatePasswd(email, passwd);
		if (!b_update) {
			std::cout << "failed to update password" << std::endl;
			root["error"] = ErrorCodes::PasswdUpdateErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "update password success" << std::endl;
		root["error"] = ErrorCodes::Success;
		root["user"] = user;
		root["email"] = email;
		root["passwd"] = passwd;
		root["verifyCode"] = verifyCode;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// ��¼����
	regPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());	// ���ͻ��˷�����������ת�����ַ�������
		std::cout << "request: user_login" << std::endl;
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

		std::string email = src_root["email"].asString();
		std::string passwd = src_root["passwd"].asString();
		UserInfo userInfo;
		// ��ѯ�û����������Ƿ�ƥ��(����о�Ӧ��������ȥ��֤�û�������Ϊ������Ψһ�ģ��û�������Ψһ�����û����������ݿ��е�������ܵõ��ü����������Ҫ���˽��)
		bool valid = MysqlMgr::getInstance()->checkPasswd(email, passwd, userInfo);
		if (!valid) {	// �û������������
			std::cout << "user not exists or user password not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "user authentication success" << std::endl;
		// ͨ��StatusServer��ѯ����ȡ��ǰ���õķ�������Ϣ���ҵ����ʵ�����
		auto reply = StatusGrpcClient::getInstance()->getChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << "grpc get chat server failed, error is: " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "login success" << std::endl;
		root["error"] = ErrorCodes::Success;
		root["uid"] = userInfo.uid;
		root["email"] = email;
		root["host"] = reply.host();
		root["port"] = reply.port();
		root["token"] = reply.token();
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