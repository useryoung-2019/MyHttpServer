#include"MysqlConnectPool.hpp"
#include"RedisConnectPool.hpp"
#include"AbstractProductSQL.hpp"
#include"AbstractFactory.hpp"
#include"FactoryRedis.hpp"
#include"FactoryMysql.hpp"
#include"ProductRedis.hpp"
#include"ProductMysql.hpp"

int main()
{
    MysqlConnectPool::getInstance()->init();
    RedisConnectPool::getInstance()->init();

    shared_ptr<AbstractFactory> mysqlFac(new FactoryMysql);
    shared_ptr<AbstractFactory> redisFac(new FactoryRedis);

    vector<pair<string, string>> mysqlData;
    string query("select user_name, user_passwd from user");
    mysqlFac->createSQL()->selectFromSQL(query, mysqlData);

    std::cout << mysqlData.size() << std::endl;

    for (auto& it : mysqlData)
    {
        string str("HSET webuser ");
        str = str + it.first + " " + it.second;
        if (!redisFac->createSQL()->IDUIntoSQL(str))
            break;
    }

    MysqlConnectPool::getInstance()->freePool();
    RedisConnectPool::getInstance()->freePool();

    return 0;
}


