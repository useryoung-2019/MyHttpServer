#ifndef TIMER_NODE_HPP
#define TIMER_NODE_HPP

#include<sys/epoll.h>
#include<unistd.h>



class TimerNode
{
private:
    int m_epollfd;
    int m_sockfd;

public:
    time_t m_timeout;
    int m_slot;

public:
    TimerNode(int epollfd = -1, int fd = -1, time_t timeout = -1):m_epollfd(epollfd),
                                                                  m_sockfd(fd),
                                                                  m_timeout(timeout),
                                                                  m_slot(0){}
    void callbackFunc()
    {
        /*epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, NULL);
        INFO(std::initializer_list<std::string>({__FILE__, ":", __func__,
            "-> ", std::to_string(m_sockfd), " is closed by server"}));
        
        if (m_sockfd != -1)
        {
            close(m_sockfd);
            m_sockfd = -1;
        }*/
    }
};

#endif