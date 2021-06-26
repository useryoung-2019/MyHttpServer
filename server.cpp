#include"server.hpp"

int Server::g_maxEvent = 10000;
int Server::g_maxSockfd = 30000;
int Server::g_extraTime = 5;

Server::Server()
{
    m_logLineSize = 20000;
    m_logIsAsync = false;
    m_logQueSize = 3000;
    
    m_threadSize = 4;
    m_taskSize = 3000;
    
    m_mysqlhost = "localhost";
    m_mysqlUser = "root";
    m_mysqlPasswd = "123456";
    m_mysqlDatebase = "web";
    m_mysqlPort = 3306;
    m_mysqlSize = 20;
    
    m_redisSize = 20;
    m_redisHost = "127.0.0.1";
    m_redisPort = 6379;

    m_epollfd = -1;
    m_listenfd = -1;
    m_timeout = false;
    m_stop = false;
}

void Server::setPipeSig()
{
    auto ret = socketpair(PF_UNIX, SOCK_STREAM, 0, g_pipeSig);
    if (ret != 0)
    {
        ERROR(LOG({__FILE__, ": failed to run socketpair, errno is", std::to_string(errno)}));
        exit(-1);
    }

    auto option = fcntl(g_pipeSig[0], F_GETFL);
    fcntl(g_pipeSig[0], F_SETFL, option|O_NONBLOCK);
    option = fcntl(g_pipeSig[1], F_GETFL);
    fcntl(g_pipeSig[1], F_SETFL, option|O_NONBLOCK);

    INFO(LOG({__FILE__, ": succeed to create socketpair"}));
}

void Server::getPara(int argc, char** argv)
{
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
        {"redisSize", required_argument, NULL, 'R'},
        {"redisIp", required_argument, NULL, 'I'},
        {"redisPort", required_argument, NULL, 'r'},
        {"threadSize", required_argument, NULL, 'H'},
        {"taskSize", required_argument, NULL, 'T'},
        {"help", no_argument, NULL, '?'},
        {0,0,0,0}
    };

    if (argc == 1)
    {
        showUsage();
        exit(-1);
    }

    int opt;
    int index = 0;
    while((opt = getopt_long(argc, argv, "L::B::C::h::u::p::d::P::R::I::r::H::T::?", longOpt, &index)) != EOF)
    {
        switch (opt)
        {
            case 'L':
            {
                m_logLineSize= atoi(optarg);
                break;
            }
            case 'B':
            {
                m_logQueSize = atoi(optarg);
                m_logIsAsync = true;
                break;
            }
            case 'C':
            {
                m_mysqlSize = atoi(optarg);
                break;
            }
            case 'h':
            {
                m_mysqlhost = optarg;
                break;
            }
            case 'u':
            {
                m_mysqlUser = optarg;
                break;
            }
            case 'p':
            {
                m_mysqlPasswd = optarg;
                break;
            }
            case 'd':
            {
                m_mysqlDatebase = optarg;
                break;
            }
            case 'P':
            {
                m_mysqlPort = atoi(optarg);
                break;
            }
            case 'R':
            {
                m_redisSize = atoi(optarg);
                break;
            }
            case 'I':
            {
                m_redisHost = optarg;
                break;
            }
            case 'r':
            {
                m_redisPort = atoi(optarg);
                break;
            }
            case 'H':
            {
                m_threadSize = atoi(optarg);
                break;
            }
            case 'T':
            {
                m_taskSize = atoi(optarg);
                break;
            }
            case '?':
            {
                showUsage();
                exit(-1);
            }
            default:
            {
                showUsage();
                exit(-1);
            }
        }
    }
}

void Server::init()
{
    m_timer = std::make_shared<Timer>();

    client.reserve(g_maxSockfd);
    for (int i = 0; i < g_maxSockfd; ++i)
        client.emplace_back(new Http());

    timerNode.reserve(g_maxSockfd);
    for (int i = 0; i < g_maxSockfd; ++i)
        timerNode.emplace_back(new TimerNode());

    try 
    {
        /*m_client = new Http[g_maxSockfd];
        m_TimerNode = new TimerNode[g_maxSockfd];*/
        m_event = new epoll_event[g_maxEvent];
    }
    catch (std::bad_alloc & e)
    {
        cerr << "epoll_event Alloc Error\n";
        exit(-1);
    }
}

void Server::initLog()
{
    Log::getInstance()->initLog(m_logLineSize, m_logIsAsync, m_logQueSize);
}

void Server::initThreadPool()
{
    if (!ThreadPool<Http>::getInstance()->init(m_threadSize, m_taskSize))
    {
        ERROR(LOG({__FILE__, ": unacceptable parameters to init threadpool!"}));
        exit(-1);
    }
}

void Server::initSQL()
{
    if (!MysqlConnectPool::getInstance()->init(m_mysqlSize, m_mysqlhost, m_mysqlUser, m_mysqlPasswd, m_mysqlDatebase, m_mysqlPort))
    {
        ERROR(LOG({__FILE__, ": unacceptable parameters to init mysqlPool!"}));
        exit(-1);
    }

    if (!RedisConnectPool::getInstance()->init(m_redisSize, m_redisHost, m_redisPort))
    {
        ERROR(LOG({__FILE__, ": unacceptable parameters to init RedisPool!"}));
        exit(-1);
    }
}

void Server::Listen()
{
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenfd == -1)
    {
        ERROR(LOG({__FILE__, ": failed to run socket(), errno is ", std::to_string(errno)}));
        exit(-1);
    }

    int sockopt = 1;
    auto ret = setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(int));
    if (ret == -1)
    {
        ERROR(LOG({__FILE__, ": failed to run setsockopt, errno is ", std::to_string(errno)}));
        exit(-1);
    }

    sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4236);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(m_listenfd, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1)
    {
        ERROR(LOG({__FILE__, ": failed to run bind, errno is ", std::to_string(errno)}));
        exit(-1);
    }

    ret = listen(m_listenfd, 5);
    if (ret == -1)
    {
        ERROR(LOG({__FILE__, ": failed to run listen, errno is ", std::to_string(errno)}));
        exit(-1);
    }

    INFO(LOG({__FILE__, ": server is ready to listen, listenfd is ", std::to_string(m_listenfd)}));
}

void Server::Epoll()
{
    m_epollfd = epoll_create(5);
    if (m_epollfd == -1)
    {
        ERROR(LOG({__FILE__, ": failed to run epoll_create, errno is ", std::to_string(errno)}));
        exit(-1);
    }
    INFO(LOG({__FILE__, ": server create EPOLLFD, epollfd is ", std::to_string(m_epollfd)}));

    Http::g_epollfd = m_epollfd;
    epoll_event ev;
    ev.data.fd = m_listenfd,
    ev.events = EPOLLIN;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &ev);

    epoll_event ev1;
    ev1.data.fd = g_pipeSig[0];
    ev1.events = EPOLLIN;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, g_pipeSig[0], &ev1);
    addSig(SIGPIPE, SIG_IGN);
    addSig(SIGALRM, sigHandler);
    addSig(SIGTERM, sigHandler);
}

bool Server::handleConnect()
{
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int connfd = accept(m_listenfd, (sockaddr*)&clientAddr, &clientLen);
    if (connfd < 0 && errno != EWOULDBLOCK)
    {
        ERROR(LOG({__FILE__, ": failed to accept, errno is ", std::to_string(errno)}));
        return false;
    }

    INFO(LOG({__FILE__, ": new client connect, fd is ", std::to_string(connfd)}));

    client[connfd]->init(connfd, clientAddr, 1);
    time_t now;
    time(&now);
    timerNode[connfd]->init(m_epollfd, connfd, now + g_extraTime);
    m_timer->addNode(timerNode[connfd]);
    return true;
}

bool Server::handleRead(int sockfd)
{
    if (client[sockfd]->Read())
    {
        if (ThreadPool<Http>::getInstance()->addTask(client[sockfd]))
        {
            m_timer->modNode(timerNode[sockfd], g_extraTime);
            return true;
        }
    }

    if (m_timer->delNode(timerNode[sockfd]))
    {
        INFO(LOG({__FILE__, ": call delNode"}));
        timerNode[sockfd]->expireHandle();
        return true;
    }

    return false;
}

bool Server::handleSignal()
{
    char sig[200];
    auto ret = recv(g_pipeSig[0], sig, 200, 0);

    if (ret <= 0)
        return false;
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            if (sig[i] == SIGTERM)
            {
                m_stop = true;
                break;
            }
            else if (sig[i] == SIGALRM)
            {
                m_timeout = true;
                break;
            }
        }
    }
    return true;
}

void Server::handleWrite(int sockfd)
{
    if (client[sockfd]->Write())
    {
        m_timer->modNode(timerNode[sockfd], g_extraTime);
    }
    else
    {
        if (m_timer->delNode(timerNode[sockfd]))
        {
            DEBUG(LOG({__FILE__, ": Write call delNode"}));
            timerNode[sockfd]->expireHandle();
        }
    }
}

void Server::destoryServer()
{
    INFO(LOG({__FILE__,": SEGV SIGTERM, time to free memory!"}));
    close(m_listenfd);
    close(m_epollfd);
    close(g_pipeSig[0]);
    close(g_pipeSig[1]);

    Log::getInstance()->freeLog();
    ThreadPool<Http>::getInstance()->freeThreadPool();
    MysqlConnectPool::getInstance()->freePool();
    RedisConnectPool::getInstance()->freePool();

    delete[] m_event;
    m_event = nullptr;
}

void Server::Loop()
{
    alarm(g_extraTime);

    while (!m_stop)
    {
        int ret = epoll_wait(m_epollfd, m_event, g_maxEvent, -1);

        if (ret < 0 && errno != EINTR)
        {
            ERROR(LOG({__FILE__, ": epoll_wait error, errno is", std::to_string(errno)}));
            break;
        }

        for (int i = 0; i < ret; ++i)
        {
            int sockfd = m_event[i].data.fd;
            uint32_t event = m_event[i].events;
            /*
            if (sockfd == m_listenfd)
            {
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);

                int connfd = accept(m_listenfd, (sockaddr*)&clientAddr, &clientLen);
                if (connfd < 0 && errno != EWOULDBLOCK)
                {
                    ERROR(LOG({__FILE__, ": failed to accept, errno is ", std::to_string(errno)}));
                    return;
                }
                INFO(LOG({__FILE__, ": new client connect, fd is ", std::to_string(connfd)}));
                client[connfd]->init(connfd, clientAddr, 1);
            }
            else if (event & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                continue;
            }
            else if (event & EPOLLIN)
            {
                client[sockfd]->Read();
                client[sockfd]->run();
            }
            else if (event & EPOLLOUT)
            {
                client[sockfd]->Write();
            }*/

            if (sockfd == m_listenfd)
            {
                handleConnect();
            }
            else if (event & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                if (m_timer->delNode(timerNode[sockfd]))
                {
                    DEBUG(LOG({__FILE__, ": Loop call delNode"}));
                    timerNode[sockfd]->expireHandle();
                }
            }
            else if ((event & EPOLLIN) && (sockfd == g_pipeSig[0]))
            {
                handleSignal();
            }
            else if (event & EPOLLIN)
            {
                handleRead(sockfd);
            }
            else if (event & EPOLLOUT)
            {
                handleWrite(sockfd);
            }
        }

        if (m_timeout)
        {
            m_timer->tick();
            INFO(LOG({__FILE__, ": run the tick"}));
            alarm(g_extraTime);
            m_timeout = false;
        }
    }
}


