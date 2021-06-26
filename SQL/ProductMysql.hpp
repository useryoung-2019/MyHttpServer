#ifndef PRODUCTMYSQL_H
#define PRODUCTMYSQL_H

#include"MysqlConnectPool.hpp"
#include"AbstractProductSQL.hpp"

class ProductMysql:public AbstractProductSQL
{
public:
    bool selectFromSQL(string& name, pair<string, string>& ret);
    bool selectFromSQL(string& str, vector<pair<string, string>>& ret);
    bool IDUIntoSQL(string& str);
};

#endif