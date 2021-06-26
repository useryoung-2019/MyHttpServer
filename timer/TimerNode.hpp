#ifndef TIMERNODE_H
#define TIMERNODE_H

#include<sys/epoll.h>
#include<unistd.h>
#include"../log/Log.hpp"

class TimerNode
{
public:
    TimerNode(int epollfd = -1, int sockfd = -1, time_t timeout = 0);
    ~TimerNode() = default;

    void init(int epollfd, int sockfd, time_t timeout);
    void expireHandle();
    int getSockfd() { return m_sockfd;}

private:
    int m_epollfd;
    int m_sockfd;

public:
    time_t m_timeout;
    int m_slot;
};

#endif