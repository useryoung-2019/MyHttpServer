/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-02
版本： v1.1
描述： BlockQueue是一个基于生产者消费者模型，利用STL非标准容器下的<queue>构造出的一个FirstIn-
      FirstOut的模板类，基本具备线程安全的特性
使用： 构造对象: BlockQueue<T> or BlockQueue<T>(int)  T为模板类， int为整型正数值
      生产者可调用： addTask(), clearTaskQueue()
      消费者可调用： takeTask(), delTask(), clearTaskQueue()
修改： 1.修复了阻塞队列会取出空值导致调试出现段错误的情况，详见takeTask修改部分
********************************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include<queue>
#include<mutex>
#include<condition_variable>
#include<cassert>

template<typename T>
class BlockQueue
{
public:
    /*
    * 函数功能： BlockQueue构造函数
    * 输入参数： blockSize 阻塞队列的最大长度
    */
    BlockQueue(int blockSize = 2000);
    ~BlockQueue();

private:
    int m_blockSize;/*阻塞队列的最大队列长度*/
    std::queue<T>* m_taskQueue;/*阻塞队列的实例化指针对象*/

    std::mutex m_mutex;/*保证线程安全的互斥量对象*/
    std::condition_variable m_cond;/*实现线程间交互的条件变量对象*/

public:
    /*
    *函数功能： 向阻塞队列中添加T类型任务
    *输入参数： task 类型T的实例化对象
    *返回值： 添加成功返回true， 否则返回false
    */
    bool addTask(const T task);

    /*
    * 函数功能： 从阻塞队列中获取任务
    * 输出参数： task
    * 返回值：  如果从阻塞队列中取出任务返回true，否则返回false
    */
    bool takeTask(T& task);

    /*
    * 函数功能： 删除阻塞队列中某个任务
    * 输入参数： NULL
    * 返回值：  void
    */
    void delTask();

    /*
    * 函数功能： 清空阻塞任务队列中所有任务
    * 输入参数： NULL
    * 返回值：  void
    */
    void clearTaskQueue();
};

template<typename T>
BlockQueue<T>::BlockQueue(int blockSize):
m_blockSize(blockSize),
m_taskQueue(NULL),
m_mutex(),
m_cond()
{
    assert(blockSize > 0);
    m_taskQueue = new std::queue<T>;
    assert(m_taskQueue != NULL);
}

template<typename T>
BlockQueue<T>::~BlockQueue()
{
    if (m_taskQueue != NULL)
    {
        delete m_taskQueue;
        m_taskQueue = NULL;
    }
}

/*
* 函数功能： 由生产者调用向阻塞队列添加任务，并立即返回
* 输入参数： task T类对象,不能为NULL或者其余空值
* 返回值：  添加成功返回true， 添加失败返回false
*/
template<typename T>
bool BlockQueue<T>::addTask(const T task)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_taskQueue->size() >= m_blockSize)
    {
        m_cond.notify_all();
        return false;
    }

    m_taskQueue->push(task);
    m_cond.notify_all();
    return true;
}

/*
* 函数功能： 由消费者调用，从阻塞队列中获取任务。
           阻塞队列非空会立即返回队列首部元素,为空则会阻塞，需要生产者唤醒
* 输出参数： task
* 返回值：  正常获取返回true，获取失败返回false
*/
template<typename T>
bool BlockQueue<T>::takeTask(T& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_taskQueue->size() <=  0)
    {
        m_cond.wait(lock);
    }

    /*此处添加额外一次判断是为了避免多线程运行该段程序导致取到空值
    *因此返回值设置为bool，防止调用该方法出现空值以致后续出现段错误
    *PS：一个线程运行该程序不会出现此类BUG，只有多线程抢夺会出现
    */
    if(m_taskQueue->size() > 0)
    {
        task = m_taskQueue->front();
        return true;
    }

    return false;
    
}

/*
* 函数功能： 由消费者调用，删除阻塞队列中的头部任务
* 输入参数： NULL
* 返回值：  NULL
*/
template<typename T>
void BlockQueue<T>::delTask()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if(m_taskQueue->size() > 0)
    {
        m_taskQueue->pop();
    }
}

/*
* 函数功能： 删除阻塞队列中所有任务
* 输入参数： NULL
* 返回值：  NULL
*/
template<typename T>
void BlockQueue<T>::clearTaskQueue()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while(m_taskQueue->size() > 0)
    {
        m_taskQueue->pop();
    }
}

#endif









