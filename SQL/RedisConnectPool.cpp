#include"RedisConnectPool.hpp"

RedisConnectPool* RedisConnectPool::getInstance()
{
    static RedisConnectPool instance;
    return &instance;
}

bool RedisConnectPool::init(size_t connectSize, string ip, int port)
{
    if (connectSize == 0 || ip.size() == 0 || port == 0)
        return false;
    
    m_connectSize = connectSize;
    m_redisQue = std::make_shared<queue<redisContext*>>();

    for (int i = 0; i < connectSize; ++i)
    {
        redisContext* client = nullptr;

        try
        {
            client = redisConnect(ip.c_str(), port);
            if (client == nullptr || client->err != 0)
            {
                if (client == nullptr)
                    throw string("Allocate client failure\n");
                else
                    throw string(client->errstr);
            }
        }
        catch (string& errmsg)
        {
            cerr << errmsg << std::endl;
            exit(-1);
        }

        m_redisQue->push(client);
    }

    INFO(LOG({__FILE__, ": RedisPool Create ", std::to_string(m_connectSize), " clients"}));
    
    return true;
}

bool RedisConnectPool::getConnect(redisContext** client)
{
    if (m_redisQue->empty())
        return false;
    
    std::unique_lock<mutex> lock(m_mutex);
    *client = m_redisQue->front();
    m_redisQue->pop();
    return true;
}

bool RedisConnectPool::freeConnect(redisContext** client)
{
    if (client == nullptr || *client == nullptr || m_redisQue->size() >= m_connectSize)
        return false;
    
    m_redisQue->push(*client);
    return true;
}

void RedisConnectPool::freePool()
{
    if (m_redisQue)
    {
        while (!m_redisQue->empty())
        {
            redisFree(m_redisQue->front());
            m_redisQue->front() = nullptr;
            m_redisQue->pop();
        }
    }
}
