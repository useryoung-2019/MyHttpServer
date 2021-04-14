#include<getopt.h>
#include"connectpool/connectpool.hpp"
#include"timer/timer.hpp"
#include"http/http.hpp"
#include"threadpool/threadpool.hpp"
#include"http/func.hpp"

typedef std::initializer_list<std::string> STR;

const int g_maxConnectSocket = 65536;
const int g_maxEventNum = 10000;
static int s_deliverSig[2];/*双向管道使用于同进程间SIGALRM通信*/

static void sigHandle(int sig)
{
    send(s_deliverSig[1], (char*)&sig, 1, 0);
}

struct option longOpt[] = 
{
    {"lineSize", required_argument, NULL, 'L'},
    {"blockSize", required_argument, NULL, 'B'},
    {"connectSize", required_argument, NULL, 'C'},
    {"host_mysql", required_argument, NULL, 'h'},
    {"account_mysql", required_argument, NULL, 'u'},
    {"password_mysql", required_argument, NULL, 'p'},
    {"datebase_mysql", required_argument, NULL, 'd'},
    {"port_mysql", required_argument, NULL, 'P'},
    {"threadSize", required_argument, NULL, 'H'},
    {"help", no_argument, NULL, '?'},
    {0,0,0,0}
};


int main(int argc, char** argv)
{
    int lineSize=500000;
    bool isAsync = false;
    int blockSize=0;
    int connectSize=5;
    string host ="localhost";
    string user="root";
    string password="123456";
    string datebase="book";
    unsigned port=3306;
    int threadSize=4;

    int opt;
    int index = 0;
    while((opt = getopt_long(argc, argv, "L::B::C::h::u::p::d::P::H::?", longOpt, &index)) != EOF)
    {
        switch (opt)
        {
            case 'L':
            {
                lineSize = atoi(optarg);
                break;
            }
            case 'B':
            {
                blockSize = atoi(optarg);
                isAsync = true;
                break;
            }
            case 'C':
            {
                connectSize = atoi(optarg);
                break;
            }
            case 'h':
            {
                host = optarg;
                break;
            }
            case 'u':
            {
                user = optarg;
                break;
            }
            case 'p':
            {
                password = optarg;
                break;
            }
            case 'd':
            {
                datebase = optarg;
                break;
            }
            case 'P':
            {
                port = atoi(optarg);
                break;
            }
            case 'H':
            {
                threadSize = atoi(optarg);
                break;
            }
            case '?':
            {
                showUsage();
                return 0;
            }
            default:
            {
                showUsage();
                return 0;
            }
        }
    }

    LogSys::getInstance()->initLogSys(lineSize, isAsync, blockSize);
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> start LogSys"}));

    ConnectPool::getInstance()->initPool(connectSize, host, user, password, datebase, port);
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> start ConnectPool"}));

    ThreadPool<Http*>* thread = new ThreadPool<Http*>(threadSize);
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> start ThreadPool"}));
    
    TimeWheel* timer = new TimeWheel;
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> start TimeWheel"}));

    Http* client = new Http[g_maxConnectSocket];
    TimerNode* node = new TimerNode[g_maxConnectSocket];
    epoll_event* events = new epoll_event[g_maxEventNum];

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> func socket failure!"}));
        return -1;
    }

    int socketopt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &socketopt, sizeof(int));

    int ret = 0;
    sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4236);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(listenfd, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> failed to run bind, errno is ",
                std::to_string(errno)}));
        return -1;
    }

    ret = listen(listenfd, 5);
    if (ret < 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> failed to run listen, errno is ",
                std::to_string(errno)}));
        return -1;
    }
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> server is ready to listen, listenfd is ",
            std::to_string(listenfd)}));

    int epollfd = epoll_create(5);
    if (epollfd < 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> failed to run epoll_create, errno is ",
                std::to_string(errno)}));
        return -1;
    }
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> epollfd is ", std::to_string(epollfd)}));
    Http::s_epollFd = epollfd;
    epoll_event ev;
    ev.data.fd = listenfd,
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, s_deliverSig) < 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> failed to run socketpair, errno is ",
                std::to_string(errno)}));
        return -1;
    }
    setNonblock(s_deliverSig[1]);
    setNonblock(s_deliverSig[0]);
    epoll_event ev1;
    ev1.data.fd = s_deliverSig[0];
    ev1.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, s_deliverSig[0], &ev1);
    addSig(SIGPIPE, SIG_IGN);
    addSig(SIGALRM, sigHandle);

    bool timeout = false;
    alarm(3);

    while(true)
    {
        int num = epoll_wait(epollfd, events, g_maxEventNum, -1);

        if (num < 0 && errno != EINTR)
        {
            ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> fail to run epoll_wait, errno is ",
                    std::to_string(errno)}));
            break;
        }
        for(int i = 0; i < num; ++i)
        {
            int sockfd = events[i].data.fd;
            uint32_t event = events[i].events;
            if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                if (timer->delNode(&node[sockfd]))
                {
                    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), " call delNode"}));
                    node[sockfd].callbackFunc();
                }
                continue;
            }
            else if ((event & EPOLLIN) && (listenfd == sockfd))
            {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int connfd = accept(listenfd, (sockaddr*)&client_addr, &client_len);

                if (connfd < 0 && errno != EWOULDBLOCK)
                {
                    ERROR(STR({__FILE__, ":", std::to_string(__LINE__), " fail to accept , errno is",
                            std::to_string(errno)}));
                    break;
                }

                if (Http::s_count >= g_maxConnectSocket)
                {
                    close(connfd);
                    continue;
                }

                INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> new client connect",
                        ", connfd is ", std::to_string(connfd)}));
                client[connfd].initSocket(connfd, client_addr, true);
                time_t now = time(&now);
                node[connfd].initNode(epollfd, connfd, now + 3);
            }
            else if ((event & EPOLLIN) && (sockfd == s_deliverSig[0]))
            {
                char signal[500];
                int ret = recv(s_deliverSig[0], signal, 500, 0);
                if (ret > 0)
                {
                    for (int i = 0; i < ret; ++i)
                    {
                        if(signal[i] == SIGALRM)
                        {
                            timeout = true;
                        }
                    }
                }
            }
            else if (event & EPOLLIN)
            {
                if (client[sockfd].readFromSocket())
                {
                    thread->addTask(&client[sockfd]);
                    timer->modNode(&node[sockfd], 10);
                }
                else
                {
                    if (timer->delNode(&node[sockfd]))
                    {
                        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), " call delNode"}));
                        node[sockfd].callbackFunc();
                    }
                }
            }
            else if (event & EPOLLOUT)
            {
                if (client[sockfd].writeToSocket())
                {
                    timer->modNode(&node[sockfd], 5);
                }
                else
                {
                    if (timer->delNode(&node[sockfd]))
                    {
                        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), " call delNode"}));
                        node[sockfd].callbackFunc();
                    }
                }
            }
        }

        if (timeout)
        {
            timer->tick();
            alarm(3);
            timeout = false;
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] events;
    delete[] node;
    delete[] client;
    delete timer;
    delete thread;
    close(s_deliverSig[1]);
    close(s_deliverSig[0]);
    return 0;
}
