#ifndef PRODUCTREDIS_H
#define PRODUCTREDIS_H

#include"AbstractProductSQL.hpp"
#include"RedisConnectPool.hpp"

class ProductRedis:public AbstractProductSQL
{
public:
    bool selectFromSQL(string& name, pair<string, string>& ret);
    bool selectFromSQL(string& str, vector<pair<string, string>>& ret);
    bool IDUIntoSQL(string& str);
};

#endif