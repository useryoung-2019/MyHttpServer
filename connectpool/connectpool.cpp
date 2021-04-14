#include"connectpool.hpp"
typedef std::initializer_list<std::string> STR;

ConnectPool* ConnectPool::s_instance = new ConnectPool;

ConnectPool::ConnectPool():m_maxConnectSize(0),
                           m_freeConnectCount(0),
                           m_mutex(),
                           m_cond(){}

/*
* 函数功能： 得到连接池类的单例指针
* 输入参数： NULL
* 返回值：  ConnectPool*指针
*/
ConnectPool* ConnectPool::getInstance()
{
    if (s_instance == NULL)
    {
        return NULL;
    }
    return s_instance;
}

/*
* 函数功能： 初始连接池各个参数
* 输入参数： maxConnectSize 连接池最大连接数量，默认为5
*          host 登录MYSQL的主机， 默认为localhost
*          user 登录MYSQL的账户，默认为root
*          password 登录MYSQL的密码，默认为123456
*          datebase 使用MYSQL的库名，默认为book
*          port 连接MYSQL的端口，默认为3306
* 返回值：  void
*/
void ConnectPool::initPool(int maxConnectSize, string host, string user,
                           string password, string datebase, unsigned port)
{
    if (maxConnectSize <= 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "wrong connect size"}));
        exit(-1);
    }

    m_maxConnectSize = maxConnectSize;

    m_host = host;
    m_user = user;
    m_password = password;
    m_datebase = datebase;
    m_port = port;

    m_mysql = new std::vector<MYSQL*>;

    if (m_mysql == NULL)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), 
                "-> initialize mysql failure"}));
        exit(-1);
    }

    m_mysql->reserve(maxConnectSize);
    for (int i = 0; i < maxConnectSize; ++i)
    {
        MYSQL* mysql = mysql_init(NULL);
        
        if (mysql == NULL)
        {
            ERROR(STR({__FILE__, ":", std::to_string(__LINE__),
                     "-> initialize mysql failure"}));
            exit(-1);
        }

        mysql = mysql_real_connect(mysql, m_host.c_str(), m_user.c_str(),
                                   m_password.c_str(), m_datebase.c_str(),
                                   m_port, NULL, 0);
        if (mysql == NULL)
        {
            ERROR(STR({__FILE__, ":", std::to_string(__LINE__),
                     "-> initialize mysql failure"}));
            exit(-1);
        }

        m_mysql->push_back(mysql);
        ++m_freeConnectCount;
    }

    INFO(STR({__FILE__, ":", std::to_string(__LINE__), 
              "-> create ", std::to_string(m_freeConnectCount), " connection"}));
}

/*
* 函数功能： 获取一个MYSQL连接，如果池内没有空闲连接则会阻塞
* 输出参数： mysql MYSQL*的引用
* 返回值：  获得连接则返回true， 未获得则返回false
*/
bool ConnectPool::getMysql(MYSQL*& mysql)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_freeConnectCount == 0)
    {
        m_cond.wait(lock);
    }

    if (m_freeConnectCount != 0)
    {
        mysql = m_mysql->back();
        m_mysql->pop_back();
        --m_freeConnectCount;
        return true;
    }
    return false;
}

/*
* 函数功能： 将已获得的MYSQL连接放入池内
* 输入参数： mysql 使用完毕的mysq连接
* 返回值：  放入池内则返回true， 若mysql为空则返回false
* 注意： mysql使用完毕后尽早释放连接
*/
bool ConnectPool::releaseMysql(MYSQL* mysql)
{
    if (mysql == NULL)
    {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_mysql->push_back(mysql);
    ++m_freeConnectCount;
    m_cond.notify_one();
    return true;
}

ConnectPool::~ConnectPool()
{
    destory();
}

/*
* 函数功能： 销毁连接池内的可释放的内存
* 输入参数： void
* 返回值：  void
*/
void ConnectPool::destory()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    MYSQL* temp;
    for(auto i = m_mysql->begin(); i != m_mysql->begin() + m_freeConnectCount; ++i)
    {
        temp = *i;
        mysql_close(temp);
    }

    delete m_mysql;
    m_freeConnectCount = 0;

    if (s_instance != NULL)
    {
        delete s_instance;
    }
}
