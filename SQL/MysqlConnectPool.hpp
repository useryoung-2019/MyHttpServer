#ifndef MYSQLCONNECTPOOL_H
#define MYSQLCONNECTPOOL_H

#include<mysql/mysql.h>
#include<queue>
#include<string>
#include<iostream>
#include<mutex>
#include<memory>
#include"../log/Log.hpp"
using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::cerr;
using std::queue;
using std::string;

class MysqlConnectPool
{
private:
    MysqlConnectPool(): m_mysqlConnSize(0) {}
    ~MysqlConnectPool() = default;
public:
    MysqlConnectPool(const MysqlConnectPool&) = delete;
    MysqlConnectPool& operator=(const MysqlConnectPool&) = delete;

    static MysqlConnectPool* getInstance();
    bool init(size_t mysqlConnSize = 20, string host = "localhost", string user = "root", string passwd = "123456", string db = "web", unsigned int port = 3306);
    bool getMYSQL(MYSQL** mysql);
    bool freeMYSQL(MYSQL** mysql);
    void freePool();

private:
    size_t m_mysqlConnSize;
    shared_ptr<queue<MYSQL*>> m_mysqlQue;
    
    static mutex g_mutex;
    static MysqlConnectPool* g_instance;
};

#endif