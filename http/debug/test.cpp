#include"../http.hpp"
#include"../../threadpool/threadpool.hpp"
#include<iostream>
using namespace std;

typedef std::initializer_list<std::string> STR;
int main()
{
    LogSys::getInstance()->initLogSys();
INFO(STR({__FILE__, ":", __func__,"-> Log Start"}));
    ConnectPool::getInstance()->initPool();
INFO(STR({__FILE__, ":", __func__, "-> ConnecPool start"}));
    ThreadPool<Http*>* threadPool = new ThreadPool<Http*>;
INFO(STR({__FILE__, ":", __func__, "-> ThreadPool start"}));
    
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4236);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
INFO(STR({__FILE__, ":", __func__, "-> Listenfd: ", std::to_string(listenfd)}));
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    bind(listenfd, (sockaddr*)&addr, sizeof(addr));
    setNonblock(listenfd);

    listen(listenfd, 5);

    int epollfd = epoll_create(5);
INFO(STR({__FILE__, ":", __func__, "-> Epollfd ", std::to_string(epollfd)}));
    Http::s_epollFd = epollfd;
    epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    epoll_event events[50];
    Http* user = new Http[50];
INFO(STR({__FILE__, ":", __func__, "-> BEGIN!"}));
    while(true)
    {
        int num = epoll_wait(epollfd, events, 50, -1);
//INFO(STR({"Epoll_wait: ", std::to_string(num)}));
        for(int i=0; i < num; ++i)
        {
            uint32_t event = events[i].events;
            int sockfd = events[i].data.fd;
INFO(STR({__FILE__, ":", __func__, "-> Event fd: ", std::to_string(sockfd)}));
            if(event & EPOLLERR || event & EPOLLHUP || event & EPOLLRDHUP)
                continue;
            else if((event & EPOLLIN) && (sockfd == listenfd))
            {
                while(1)
                {
                    sockaddr_in client;
                    socklen_t len = sizeof(client);

                    int fd = accept(listenfd, (sockaddr*)&client, &len);
cout << fd << endl;
                    if(fd == -1 && errno == EWOULDBLOCK)
                        break;
INFO(STR({__FILE__, ":", __func__, "-> new sockfd: ", std::to_string(fd)}));
                    user[fd].initSocket(fd, client, true);
                }
                continue;
            }
            else if(event & EPOLLIN)
            {
                user[sockfd].readFromSocket();
                threadPool->addTask(user + sockfd);
            }
            else if(event & EPOLLOUT)
            {
                user[sockfd].writeToSocket();
            }
        }
    }
    
    close(epollfd);
    close(listenfd);
    delete[] user;
    delete threadPool;
    return 0;
}