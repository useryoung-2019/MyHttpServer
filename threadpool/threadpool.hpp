/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-03
版本： v1.0
描述： 基于c++11的部分特性写下的线程池模板类。
使用： 仅仅需要主线程构造一个线程池即可调用线程池工作，需要主线程自行new一类对象[T为class*]
      并向线程池添加，线程池自身不对外部做内存释放工作，需要主线程自行释放。
      主线程仅需调用ThreadPool(), addTask()两种函数，并且task任务必须要有一个回调函数：
      格式为 void run();
********************************************************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<thread>
#include<mutex>
#include<condition_variable>
#include<vector>
#include<list>
#include<cassert>
#include"../log/log_sys.hpp"

typedef std::initializer_list<std::string> STR;

template<typename T>
class ThreadPool
{
public:
    ThreadPool(int threadSize = 4);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    int m_threadSize;/*线程池中最大线程数量*/
    bool m_isStart;/*线程池是否运行的标识*/

    std::mutex m_mutex;/*保持线程安全的互斥量*/
    std::condition_variable m_cond;/*线程间通信的条件变量*/

    std::vector<std::thread*>* m_thread;/*存储已初始化完毕的线程的容器*/
    std::list<T>* m_task;/*存储线程需要运行的任务容器*/

public:
    void addTask(const T task);

private:
    void startPool();
    bool takeTask(T& task);
    void threadLoop();
    void stopPool();
};

/*
* 函数功能： 构造函数，调用后即开启线程池
* 输入参数： threadSize 线程池中的线程数，默认值为4
*/
template<typename T>
ThreadPool<T>::ThreadPool(int threadSize):m_threadSize(threadSize),
                                          m_isStart(false),
                                          m_mutex(),
                                          m_cond(),
                                          m_thread(NULL),
                                          m_task(NULL)
{
    assert(m_threadSize > 0);
    startPool();
}

/*
* 函数功能： 初始化线程池的相关参数，由构造函数调用，并且开始运行线程池
* 输入参数： NULL
* 返回值：  void
*/
template<typename T>
void ThreadPool<T>::startPool()
{
    if (m_thread != NULL)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), 
                "-> exist one instance of threadpool"}));
        exit(-1);
    }

    m_isStart = true;
    m_thread = new std::vector<std::thread*>;
    m_task = new std::list<T>;
    
    if (m_task == NULL || m_thread == NULL)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), 
                "-> fail to initialize pool"}));
        exit(-1);
    }

    m_thread->reserve(m_threadSize);
    for (int i = 0; i < m_threadSize; ++i)
    {
        m_thread->push_back(new std::thread(&ThreadPool::threadLoop, this));
    }
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> create ", 
                std::to_string(m_thread->size()), " threads"}));
}

/*
* 函数功能： 线程执行的线程循环方法
* 输入参数： NULL
* 返回值：  void
* 提醒：    若任务类型为对象，需要修改此函数中的task调用回调函数的部分，改为task.run()
*/
template<typename T>
void ThreadPool<T>::threadLoop()
{   
    T task;
    while(m_isStart)
    {
        /*DEBUG(STR({"list size is ", std::to_string(m_task->size())}));*/
        bool getTask = takeTask(task);
        /*DEBUG(STR({"get task!"}));*/
        if (!m_isStart)/*运行标识为false就说明该退出线程，在主线程调用析构被回收*/
        {
            return;
        }

        if (!getTask)/*返回false只有两种情况，一种未取到值，此时需要继续循环工作*/
        {            /*另一种是该退出线程，此时再进行一次循环判断即可退出*/   
            continue;
        }
        else
        {
            task->run();
        }
    }
}

/*
* 函数功能： 线程池自身调用该函数，获得任务
* 输出参数： task 类T的对象的引用，获取任务的存储对象
* 返回值：  成功获得任务返回true，并且将任务赋予task中， 否则返回false，task无变化
*/
template<typename T>
bool ThreadPool<T>::takeTask(T& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_task->size() == 0)
    {
        m_cond.wait(lock);
    }

    if (m_isStart && m_task->size() > 0)
    {
        task = m_task->front();
        m_task->pop_front();
        return true;
    }
    return false;
}

/*
* 函数功能： 主线程调用该函数，向线程池中添加任务
* 输入参数： task 类T对象
* 返回值：  void
*/
template<typename T>
void ThreadPool<T>::addTask(const T task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    m_task->push_back(task);
    m_cond.notify_one();
}

template<typename T>
ThreadPool<T>::~ThreadPool()
{
    if (m_isStart)
    {
        stopPool();
    }
}

/*
* 函数功能： 由析构函数调用，其余情况禁止调用，销毁线程池中所有参数，但任务队列中的任务不会销毁
            需要主线程自行销毁，否则可能出现内存泄漏的风险
* 输入参数： NULL
* 返回值：  void
*/
template<typename T>
void ThreadPool<T>::stopPool()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isStart = false;
    m_cond.notify_all();

    m_task->clear();
    delete m_task;
    m_task = NULL;

    for (auto i : *m_thread)
    {
        i->join();
        delete i;
    }
    delete m_thread;
    m_thread = NULL;
}

#endif