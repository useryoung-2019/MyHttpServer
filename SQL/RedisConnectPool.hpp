#ifndef REDISCONNECTPOOL_H
#define REDISCONNECTPOOL_H

#include<hiredis/hiredis.h>
#include<queue>
#include<string>
#include<memory>
#include<iostream>
#include<mutex>
#include"../log/Log.hpp"
using std::mutex;
using std::cerr;
using std::string;
using std::queue;
using std::shared_ptr;

class RedisConnectPool
{
private:
    RedisConnectPool():m_connectSize(0), m_mutex(){}
    ~RedisConnectPool() = default;

public:
    RedisConnectPool(const RedisConnectPool&) = delete;
    RedisConnectPool& operator=(const RedisConnectPool&) = delete;

    bool init(size_t connectSize = 20, string ip = "127.0.0.1", int port = 6379);
    bool getConnect(redisContext** client);
    bool freeConnect(redisContext** client);
    void freePool();
    static RedisConnectPool* getInstance();

private:

    size_t m_connectSize;
    mutex m_mutex;
    shared_ptr<queue<redisContext*>> m_redisQue;
};

#endif