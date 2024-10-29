#pragma once
// MysqlMgrΪ�����࣬��������MysqlDao�ĸ���
#include "const.h"
#include "MysqlDao.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int regUser(const std::string& name, const std::string& email, const std::string& pwd);
private:
    MysqlMgr();
    MysqlDao  _dao;
};

