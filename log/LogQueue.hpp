

#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include<mutex>
#include<condition_variable>
#include<iostream>
#include<queue>
#include<memory>

using std::shared_ptr;
using std::unique_ptr;
using std::queue;
using std::mutex;
using std::condition_variable;

template<typename T>
class LogQueue
{
public:
    LogQueue(size_t queueSize = 2000);
    ~LogQueue();

public:
    bool addTask(const shared_ptr<T>& task);
    shared_ptr<T> getTask();
    void clearTask();

private:
    size_t m_queueSize;
    shared_ptr<queue<shared_ptr<T>>> m_taskQueue;
    mutex m_mutex;
    condition_variable m_cond;
    bool m_quitQue;
};

template<typename T>
LogQueue<T>::LogQueue(size_t queueSize):m_queueSize(queueSize),
                                        m_mutex(),
                                        m_cond(),
                                        m_quitQue(false)
{
    if (queueSize <= 0)
    {
        std::cerr << "Unreceptable number to construct logQueue!\n";
        exit(-1);
    }
    m_taskQueue = std::make_shared<queue<shared_ptr<T>>>(queue<shared_ptr<T>>());
}

template<typename T>
LogQueue<T>::~LogQueue()
{
    if (m_taskQueue)
        clearTask();
}

template<typename T>
bool LogQueue<T>::addTask(const shared_ptr<T>& task)
{
    if (m_taskQueue->size() >= m_queueSize)
    {
        m_cond.notify_all();
        return false;
    }

    if (task)
    {
        std::unique_lock<mutex> lock(m_mutex);
        m_taskQueue->push(task);
        m_cond.notify_all();
        return true;
    }
    else
        return false;
}

template<typename T>
shared_ptr<T> LogQueue<T>::getTask()
{
    std::unique_lock<mutex> lock(m_mutex);
    
    while (m_taskQueue->size() <= 0)
    {
        m_cond.wait(lock);
        if (m_quitQue)
            return std::make_shared<T>();
    }

    auto ret = m_taskQueue->front();
    m_taskQueue->pop();
    return ret;
}

template<typename T>
void LogQueue<T>::clearTask()
{
    if (m_taskQueue)
    {
        std::unique_lock<mutex> lock(m_mutex);
        m_quitQue = true;
        m_cond.notify_all();

        while (!m_taskQueue->empty())
        {
            m_taskQueue->pop();
        }
    }
}

#endif