#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::regUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return _dao.regUser(name, email, pwd);
}

MysqlMgr::MysqlMgr() {
}