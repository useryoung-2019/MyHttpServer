#include"TimerNode.hpp"

TimerNode::TimerNode(int epollfd, int sockfd, time_t timeout):
m_epollfd(epollfd),
m_sockfd(sockfd),
m_timeout(timeout),
m_slot(-1){}

void TimerNode::init(int epollfd, int sockfd, time_t timeout)
{
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_timeout = timeout;
    DEBUG(LOG({__FILE__, ": ", std::to_string(sockfd), " ", std::to_string(timeout)}));
}

void TimerNode::expireHandle()
{
    if (m_epollfd <= 0 || m_sockfd <= 0)
        return;
    
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, nullptr);
    INFO(LOG({__FILE__, ": ", std::to_string(m_sockfd),
              "is closed by server!"}));

    if (m_sockfd > 0)
    {
        close(m_sockfd);
    }
}