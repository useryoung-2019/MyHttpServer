#include"SQL/ConnectPool.hpp"
#include"log/Log.hpp"
#include"http/Http.hpp"
#include"ThreadPool/ThreadPool.hpp"
#include"timer/Timer.hpp"
#include<getopt.h>

static int g_pipeSig[2];

static void sigHandler(int sig)
{
    send(g_pipeSig[1], (char*)(&sig), 1, 0);
}

class Server
{
public:
    typedef shared_ptr<Http> pHttp;
    typedef shared_ptr<TimerNode> pTimerNode;

public:
    Server();
    ~Server() = default;

    void setPipeSig();

    void getPara(int argc, char** argv);
    void init();
    void initLog();
    void initThreadPool();
    void initSQL();

    void Listen();
    void Epoll();

    void Loop();
    void destoryServer();

private:
    bool handleConnect();
    bool handleSignal();
    bool handleRead(int sockfd);
    void handleWrite(int sockfd);

private:
    static int g_maxSockfd;
    static int g_maxEvent;
    static int g_extraTime;

private:
    size_t m_logLineSize;
    size_t m_logQueSize;
    bool m_logIsAsync;

    size_t m_threadSize;
    size_t m_taskSize;

    size_t m_mysqlSize;
    string m_mysqlhost;
    string m_mysqlUser;
    string m_mysqlPasswd;
    string m_mysqlDatebase;
    unsigned m_mysqlPort;

    size_t m_redisSize;
    string m_redisHost;
    size_t m_redisPort;

    int m_epollfd;
    int m_listenfd;

    vector<pHttp> client;
    vector<pTimerNode> timerNode;
    epoll_event* m_event;
    shared_ptr<Timer> m_timer;

    bool m_timeout;
    bool m_stop;
};


