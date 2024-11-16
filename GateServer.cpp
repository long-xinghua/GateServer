// GateServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"


void TestRedis() {
	//连接redis 需要启动才可以进行连接
//redis默认监听端口为6387 可以再配置文件中修改
	redisContext* c = redisConnect("127.0.0.1", 6380);
	if (c->err)	// err不为0即有错
	{
		printf("Connect to redisServer faile:%s\n", c->errstr);
		redisFree(c);        return;
	}
	printf("Connect to redisServer Success\n");

	//std::string redis_password = "123456";
	const char* redis_password = "123456";	// 要用char*类型的字符串，不能用string类型
	redisReply* r = (redisReply*)redisCommand(c, "AUTH %s", redis_password);	// 给redis发命令，得到应答r
	if (r->type == REDIS_REPLY_ERROR) {
		printf("Redis认证失败！\n");
	}
	else {
		printf("Redis认证成功！\n");
	}

	//为redis设置key
	const char* command1 = "set stest1 value1";

	//执行redis命令行
	r = (redisReply*)redisCommand(c, command1);

	//如果返回NULL则说明执行失败
	if (NULL == r)
	{
		printf("Execut command1 failure\n");
		redisFree(c);        return;
	}

	//如果执行失败则释放连接
	if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
	{
		printf("Failed to execute command[%s]\n", command1);
		freeReplyObject(r);
		redisFree(c);        return;
	}

	//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command1);

	const char* command2 = "strlen stest1";
	r = (redisReply*)redisCommand(c, command2);

	//如果返回类型不是整形 则释放连接
	if (r->type != REDIS_REPLY_INTEGER)
	{
		printf("Failed to execute command[%s]\n", command2);
		freeReplyObject(r);
		redisFree(c);        return;
	}

	//获取字符串长度
	int length = r->integer;
	freeReplyObject(r);
	printf("The length of 'stest1' is %d.\n", length);
	printf("Succeed to execute command[%s]\n", command2);

	//获取redis键值对信息
	const char* command3 = "get stest1";	// 该命令获取stest1所对应的value值
	r = (redisReply*)redisCommand(c, command3);	
	if (r->type != REDIS_REPLY_STRING)	// 返回值不是string就说明失败了
	{
		printf("Failed to execute command[%s]\n", command3);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	printf("The value of 'stest1' is %s\n", r->str);
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command3);

	const char* command4 = "get stest2";
	r = (redisReply*)redisCommand(c, command4);
	if (r->type != REDIS_REPLY_NIL)	// 由于没有在redis里设置stest2，正常情况下应返回空值
	{
		printf("Failed to execute command[%s]\n", command4);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command4);

	//释放连接资源
	redisFree(c);

}

void TestRedisMgr() {
	//assert(RedisMgr::getInstance()->connect("127.0.0.1", 6380));
	//assert(RedisMgr::getInstance()->auth("123456"));
	assert(RedisMgr::getInstance()->set("blogwebsite", "llfc.club"));
	std::string value = "";
	assert(RedisMgr::getInstance()->get("blogwebsite", value));
	assert(RedisMgr::getInstance()->get("nonekey", value) == false);
	assert(RedisMgr::getInstance()->hSet("bloginfo", "blogwebsite", "llfc.club"));
	assert(RedisMgr::getInstance()->hGet("bloginfo", "blogwebsite") != "");
	assert(RedisMgr::getInstance()->existsKey("bloginfo"));
	assert(RedisMgr::getInstance()->del("bloginfo"));
	assert(RedisMgr::getInstance()->del("bloginfo"));
	assert(RedisMgr::getInstance()->existsKey("bloginfo") == false);
	assert(RedisMgr::getInstance()->lPush("lpushkey1", "lpushvalue1"));
	assert(RedisMgr::getInstance()->lPush("lpushkey1", "lpushvalue2"));
	assert(RedisMgr::getInstance()->lPush("lpushkey1", "lpushvalue3"));
	assert(RedisMgr::getInstance()->rPop("lpushkey1", value));
	assert(RedisMgr::getInstance()->rPop("lpushkey1", value));
	assert(RedisMgr::getInstance()->lPop("lpushkey1", value));
	assert(RedisMgr::getInstance()->lPop("lpushkey2", value) == false);
	//RedisMgr::getInstance()->close(); 在RedisMgr的析构中会调用
}

int main()
{
	//TestRedis();
	//TestRedisMgr();
	auto& gCfgMgr = ConfigMgr::getInst();	// 存放的配置数据(由于单例类没有拷贝构造，这里就用引用的方式传递)
	std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
	unsigned short gate_port = atoi(gate_port_str.c_str());

	try {
		unsigned short port = static_cast<unsigned short>(8080);	// 设置端口
		net::io_context ioc{ 1 };	// 底层设置一个线程来跑
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);	//指定捕获SIGINT(ctrl+C的信号)和SIGTERM(系统或其他进程发送的终止信号)信号
		signals.async_wait([&ioc](const boost::system::error_code &error, int signal_number) {	//触发信号调用回调函数
			if (error) {
				return;
			}
			ioc.stop();
			});

		std::make_shared<CServer>(ioc, port)->start();	// 创建CServer对象，绑定底层ioc和端口并让它跑起来
		std::cout << "Gate Serverlisten on port " << port << std::endl;
		ioc.run();	//让事件轮询起来

	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
