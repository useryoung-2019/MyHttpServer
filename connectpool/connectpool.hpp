/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-03
版本： v1.0
描述： 基于c++11、mysql c/c++ api、以及饿汉单例模式下的简易连接池
使用： 连接池的正常使用必须调用initPool函数以初始化连接池
      当需要mysql连接时调用getMysql，使用完毕后必须调用releaseMysql，否则导致后期长期阻塞
      得不到MYSQL连接,详细使用见DEBUG文件夹下的cpp文件
********************************************************************************/
#ifndef CONNECTPOOL_H
#define CONNECTPOOL_H

#include<mysql/mysql.h>
#include<string>
#include<vector>
#include<mutex>
#include<condition_variable>
#include"../log/log_sys.hpp"

using std::string;

class ConnectPool
{
private:
    int m_maxConnectSize;/*连接池中最大连接数量*/
    int m_freeConnectCount;/*连接池中的空闲连接*/
    std::vector<MYSQL*>* m_mysql;/*存储连接的容器*/

    string m_host;/*登录MYSQL的主机*/
    string m_user;/*登录MYSQL的账户*/
    string m_password;/*登录MYSQL的密码*/   
    string m_datebase;/*需要使用MYSQL的数据库*/
    unsigned int m_port;/*api连接需要的端口*/
    
    std::mutex m_mutex;/*保持线程安全的互斥量*/
    std::condition_variable m_cond;/*线程间通信的条件变量*/

    static ConnectPool* s_instance;/*饿汉单例模式*/

private:
    ConnectPool();
    ~ConnectPool();

    /*
    * 函数功能： 销毁连接池内的可释放的内存
    * 输入参数： void
    * 返回值：  void
    */
    void destory();
public:
    ConnectPool(const ConnectPool&) = delete;
    ConnectPool& operator=(const ConnectPool&) = delete;

    /*
    * 函数功能： 得到连接池类的单例指针
    * 输入参数： NULL
    * 返回值：  ConnectPool*指针
    */
    static ConnectPool* getInstance();

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
    void initPool(int maxConnectSize = 5, string host = "localhost", string user = "root",
                  string password = "123456", string datebase = "book", unsigned port = 3306);
    
    /*
    * 函数功能： 获取一个MYSQL连接
    * 输出参数： mysql MYSQL*的引用
    * 返回值：  获得连接则返回true， 未获得则返回false
    */
    bool getMysql(MYSQL*& mysql);

    /*
    * 函数功能： 将已获得的MYSQL连接放入池内
    * 输入参数： mysql 使用完毕的mysq连接
    * 返回值：  放入池内则返回true， 若mysql为空则返回false，
    */
    bool releaseMysql(MYSQL* mysql);
};

#endif