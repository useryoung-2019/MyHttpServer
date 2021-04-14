/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-07
版本： v1.0
描述： 调用定时器的基础类
********************************************************************************/
#ifndef TIMER_NODE_HPP
#define TIMER_NODE_HPP

#include<sys/epoll.h>
#include<unistd.h>
#include"../log/log_sys.hpp"


class TimerNode
{
private:
    int m_epollfd;/*m_epollfd句柄*/
    int m_sockfd;/*sockfd*/

public:
    time_t m_timeout;/*m_sockfd断开的时间*/
    int m_slot;/*该对象在时间轮上的时间块的位置*/

public:
    TimerNode()
    {
        m_epollfd = -1;
        m_sockfd = -1;
        m_timeout = 0;
        m_slot = 0;
    }
    
    void initNode (int epollfd, int sockfd, time_t timeout)
    {
        m_epollfd = epollfd;
        m_sockfd = sockfd;
        m_timeout = timeout;
        m_slot = 0;
    }
    
    /*
    * 函数功能： 销毁时间点需要执行的函数，主动断开连接
    * 输入参数： void
    * 返回值：  void
    */
    void callbackFunc()
    {
        if (m_epollfd <= 0)
        {
            return;
        }

        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, NULL);
        INFO(std::initializer_list<std::string>({__FILE__, ":", std::to_string(__LINE__),
            "-> ", std::to_string(m_sockfd), " is closed by server"}));
        
        if (m_sockfd != -1)
        {
            close(m_sockfd);
            m_sockfd = -1;
            m_timeout = 0;
            m_epollfd  = -1;
            m_slot = 0;
        }
    }
};

#endif