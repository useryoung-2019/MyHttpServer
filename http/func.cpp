#include<iostream>
#include"func.hpp"

void setNonblock(int fd)
{
    int option = fcntl(fd, F_GETFL);
    option |= O_NONBLOCK;
    fcntl(fd, F_SETFL, option);
}

void epollAddfd(int epollfd, int fd, bool modeET)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;

    if (modeET)
    {
        event.events |= EPOLLET;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonblock(fd);
}

void epollDelfd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void epollModfd(int epollfd, int fd, int ev, bool modeET)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP | EPOLLONESHOT;

    if (modeET)
    {
        event.events |= EPOLLET;
    }

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void addSig(int sig, void (*handle)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handle;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

void showUsage()
{
    std::cout << "This is a tiny http server made by HAPPY YOUNG\n"
         << "Please configure your operatoring parameters!\n\n"
         << "      opt                   explain               instance    default\n"
         << "-L|--lineSize       max line of a log file         -L3000     20000\n"
         << "-B|--blockSize      Async log's block size         -B3000     0\n"
         << "-C|--connectSize    max number of connectpool      -C30       20\n"
         << "-h|--host_mysql     mysql'host                     -hyoung    localhost\n"
         << "-u|--account_mysql  load mysql's account           -uhil      root\n"
         << "-p|--password_mysql load mysql's password          -p12333    123456\n"
         << "-d|--datebase_mysql mysql's datebase               -dhjkl     web\n"
         << "-P|--port_mysql     connect tp mysql's port        -P1234     3306\n"
         << "-R|--redisSize      redis max client               -R20       20\n"
         << "-I|--redisIp        redis server ip               -I127.0.0.1 127.0.0.1\n"
         << "-r|--redisPort      redis port                     -r6379     6379\n"
         << "-H|--threadSize     max number of thread           -H24       4\n"
         << "-T|--taskSize       max task block size            -T20       3000\n"
         << "-?|--help           help                           -?\n";

}