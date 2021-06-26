#ifndef FUNC_HPP
#define FUNC_HPP

#include<unistd.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<sys/signal.h>
#include<cstring>
#include<cassert>
/*
* 函数功能： 将fd设置为非阻塞
* 输入参数： fd 文件描述符
* 返回值：  void
*/
void setNonblock(int fd);

/*
* 函数功能： 向EPOLL句柄中注册事件
* 输入参数： epollfd 句柄
            fd  需要向句柄中注册事件的fd
            modeET 是否同时修改触发模式，true为ET
* 返回值：  void
*/
void epollAddfd(int epollfd, int fd, bool modeET);

/*
* 函数功能： 从EPOLL句柄中删除事件
* 输入参数： epollfd  句柄
            fd  文件描述符
* 返回值：  void
*/
void epollDelfd(int epollfd, int fd);

/*
* 函数功能： 修改fd在EPOLLFD上的注册事件
* 输入参数： epollfd 句柄
            fd 需要修改注册事件的文件描述符
            ev 需要注册的事件
            modeET 是否同时加入ET触发模式
* 返回值：  void
*/
void epollModfd(int epollfd, int fd, int ev, bool triggerET);

/*
* 函数功能： 信号处理函数
* 输入参数： sig  信号值
           handle  处理信号的回调函数
* 返回值：  void
*/
void addSig(int sig, void (*handle)(int));

/*
* 函数功能： 参数设置展示
*/
void showUsage();

#endif