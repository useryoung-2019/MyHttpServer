#include"MysqlConnectPool.hpp"
MysqlConnectPool* MysqlConnectPool::g_instance = nullptr;
mutex MysqlConnectPool::g_mutex;

MysqlConnectPool* MysqlConnectPool::getInstance()
{
    if (g_instance == nullptr)
    {
        std::unique_lock<mutex> lock(g_mutex);
        if (g_instance == nullptr)
        {
            g_instance = new MysqlConnectPool;
        }
    }

    return g_instance;
}

bool MysqlConnectPool::init(size_t mysqlConnSize, string host, string user, string passwd, string db, unsigned int port)
{
    if (mysqlConnSize <= 0 || host.size() == 0 || user.size() == 0 || passwd.size() == 0 || db.size() == 0 || port == 0)
        return false;
    
    m_mysqlConnSize = mysqlConnSize;
    m_mysqlQue = std::make_shared<queue<MYSQL*>>();
    
    for (int i = 0; i < mysqlConnSize; ++i)
    {
        MYSQL* mysql = new MYSQL;
        
        try
        {
            if (mysql_init(mysql) == nullptr)
                throw string("Err:mysql_init failed\n");
            
            if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                throw string(mysql_error(mysql));
        }
        catch(string& errmsg)
        {
            cerr << errmsg << std::endl;
            exit(-1);
        }

        m_mysqlQue->push(mysql);
    }
    INFO(LOG({__FILE__, ": MysqlPool Create ", std::to_string(m_mysqlConnSize), " clients"}));
    return true;
}

bool MysqlConnectPool::getMYSQL(MYSQL** mysql)
{
    if (m_mysqlQue->empty())
        return false;
    std::unique_lock<mutex> lock(g_mutex);
    *mysql = m_mysqlQue->front();
    m_mysqlQue->pop();
    return true;
}

bool MysqlConnectPool::freeMYSQL(MYSQL** mysql)
{
    if (mysql == nullptr || *mysql == nullptr || m_mysqlQue->size() >= m_mysqlConnSize)
        return false;
    
    m_mysqlQue->push(*mysql);
    return true;
}

void MysqlConnectPool::freePool()
{
    if (m_mysqlQue)
    {
        while (!m_mysqlQue->empty())
        {
            mysql_close(m_mysqlQue->front());
            delete m_mysqlQue->front();
            m_mysqlQue->front() = nullptr;
            m_mysqlQue->pop();
        }
    }

    mysql_library_end();

    if (g_instance != nullptr)
    {
        delete g_instance;
        g_instance = nullptr;
    }
}
