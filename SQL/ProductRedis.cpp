#include"ProductRedis.hpp"

bool ProductRedis::selectFromSQL(string& name, pair<string, string>& ret)
{
    redisContext* client = nullptr;
    string str("HGET wenuser ");
    str += name;
    
    if (RedisConnectPool::getInstance()->getConnect(&client))
    {
        redisReply* reply = static_cast<redisReply*>(redisCommand(client, str.c_str()));
        
        if (reply->type != 1)
        {
            freeReplyObject(reply);
            RedisConnectPool::getInstance()->freeConnect(&client);
            return false;
        }
        
        ret.first = name;
        ret.second = reply->element[1]->str;

        freeReplyObject(reply);
        RedisConnectPool::getInstance()->freeConnect(&client);
        client = nullptr;
        return true;
    }

    return false;
}

bool ProductRedis::selectFromSQL(string& str, vector<pair<string, string>>& ret)
{
    redisContext* client = nullptr;
    ret.clear();
    
    if (RedisConnectPool::getInstance()->getConnect(&client))
    {
        redisReply* reply = static_cast<redisReply*>(redisCommand(client, str.c_str()));
        
        if (reply->type != 2)
        {
            freeReplyObject(reply);
            RedisConnectPool::getInstance()->freeConnect(&client);
            client = nullptr;
            return false;
        }
        
        
        ret.reserve(reply->elements >> 1);
        for (size_t i = 0; i < reply->elements; i += 2)
        {
            string fir = reply->element[i]->str;
            string sec = reply->element[i+1]->str;
            ret.emplace_back(fir, sec);
        }

        freeReplyObject(reply);
        RedisConnectPool::getInstance()->freeConnect(&client);
        client = nullptr;

        return true;
    }

    return false;
}

bool ProductRedis::IDUIntoSQL(string& str)
{
    redisContext* client = nullptr;
    
    if (RedisConnectPool::getInstance()->getConnect(&client))
    {
        redisReply* reply = static_cast<redisReply*>(redisCommand(client, str.c_str()));

        if (reply->type == 6)
        {
            freeReplyObject(reply);
            RedisConnectPool::getInstance()->freeConnect(&client);
            client = nullptr;
            return false;
        }

        freeReplyObject(reply);
        RedisConnectPool::getInstance()->freeConnect(&client);
        client = nullptr;

        return true;
    }

    return false;
}