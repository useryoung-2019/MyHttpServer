/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-02
版本： v1.0
描述： LogSys是一个基于c++11的static线程安全单例模式【懒汉单例】，并且根据需求可设置同步/异步
      两种模式，另可根据需要设置单个log文件显示的行数。异步模式下嵌入了BlockQueue类，以消费者
      生产者模式异步操作。
使用： 必须调用initLogSYS()以确定异步/同步模式，初始化之后即可利用函数宏调用进行log输出
********************************************************************************/
#ifndef LOG_SYS_H
#define LOG_SYS_H

#include<string>
#include<initializer_list>
#include<cassert>
#include<ctime>
#include<thread>
#include<fstream>
#include"block_queue.hpp"

using std::string;

class LogSys
{
public:
    /*log输出的四个等级，DEBUG， INFO， WARN， ERROR*/
    enum LOG_LEVEL { DEBUG = 0,
                     INFO, 
                     WARN, 
                     ERROR };

public:
    LogSys(const LogSys&) = delete;
    LogSys& operator= (const LogSys&) = delete;

    /*
    * 函数功能： 获得LogSys类的实例对象的地址
    * 输入参数： NULL
    * 返回值：  返回LogSys对象的地址
    */
    static LogSys* getInstance();

    /*
    * 函数功能： 初始化LogSys下的各个参数
    * 输入参数： maxLineSize 单个日志文件最大行数
    * 输入参数： logIsAsync 是否开启异步模式， true为开启
    * 输入参数： blockSize 异步模式下阻塞队列大小
    * 返回值：  void
    */
    void initLogSys(int maxLineSize = 50000, bool logIsAsync = false, int blockSize = 0);

    /*
    * 函数功能： 将initializer_list<string>对象以文件读写模式输出
    * 输入参数： level 枚举类型LOG_LEVEL下的四种模式
    * 输入参数： str 列表初始化类的对象的右值引用
    * 返回值：  void
    */
    void output(LOG_LEVEL level, std::initializer_list<string>&& str);

private:
    LogSys();
    ~LogSys();

    /*
    * 函数功能： 获取当前运行的时间日期，以string的形式传出
    * 输入参数： NULL
    * 输出参数： date 当前运行的日期，string对象 YYYYMMDD
    * 输出参数： clock 当前运行的时间，string对象 HH：MM：SS
    * 返回值：  void
    */
    void getDate(string& date, string& clock);
    
    /*
    * 函数功能： 异步模式下日志输出的线程运行函数，向日志文件中写日志
    * 输入参数： NULL
    * 返回值：  void
    */
    void threadLoop();

private:
    string m_dateRecord;/*记录运行的日期，yyyymmdd*/
    string m_timeRecord;/*记录运行的时间，hh：mm：ss*/
    string m_fileName;/*需要输出的文件名，yyyymmdd-N.log*/

    unsigned int m_lineCount;/*记录在当前文件中已输出的行数*/
    unsigned int m_maxLineSize;/*log文件中能保存的最大行数*/
    unsigned int m_logPage;/*存储文件输出的个数*/

    std::mutex m_mutex;/*维护线程安全的互斥量*/
    std::ofstream m_outputFile;/*c++文件操作对象*/

private:
    bool m_logIsAsync;/*同步/异步开启的标志*/
    int m_blockSize;/*异步运行时的阻塞队列大小*/
    BlockQueue<string*>* m_blockQueue;/*异步运行时需要设置的阻塞队列对象实例*/
    std::thread* m_logThread;  /*异步运行时需要的消费者线程*/
};

/*
* 函数功能： 宏函数，便于程序书写
* 输入参数： text initializer_list<string>的右值引用
* 返回值：  void
*/
#define DEBUG(text) LogSys::getInstance()->output(LogSys::DEBUG, text)
#define INFO(text) LogSys::getInstance()->output(LogSys::INFO, text)
#define WARN(text) LogSys::getInstance()->output(LogSys::WARN, text)
#define ERROR(text) LogSys::getInstance()->output(LogSys::ERROR, text)

#endif
