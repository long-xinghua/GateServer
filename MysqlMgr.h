#pragma once
// MysqlMgr为单例类，当作调用MysqlDao的父层
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

