#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<vector>
#include<string>
#include<memory>
#include<stdexcept>
#include<functional>
#include<condition_variable>
#include"../log/Log.hpp"

using std::vector;

template<typename T>
class ThreadPool
{
private:
    ThreadPool();
    void process();
    shared_ptr<T> takeTask();

public:
    ~ThreadPool() = default;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    bool init(size_t threadSize = 4, size_t m_taskQueueSize = 100);
    
    static ThreadPool* getInstance()
    {
        return g_instance;
    }

    bool addTask(const shared_ptr<T>& task);
    void freeThreadPool();
    
private:
    size_t m_threadSize;
    bool m_start;
    vector<thread*>* m_thread;
    mutex m_mutex;
    condition_variable m_cond;

    size_t m_taskQueueSize;
    queue<shared_ptr<T>>* m_taskQueue;

    static ThreadPool* g_instance;
};

template<typename T> ThreadPool<T>* ThreadPool<T>::g_instance = new ThreadPool;

template<typename T>
ThreadPool<T>::ThreadPool():m_mutex(), m_cond()
{
    m_threadSize = 0;
    m_thread = nullptr;
    m_start = false;
    m_taskQueueSize = 0;
    m_taskQueue = nullptr;
}

template<typename T>
bool ThreadPool<T>::init(size_t threadSize, size_t taskQueueSize)
{
    if (threadSize == 0 || taskQueueSize == 0)
        return false;
    
    m_start = true;

    try
    {
        m_taskQueueSize = taskQueueSize;
        m_taskQueue = new queue<shared_ptr<T>>;
        m_threadSize = threadSize;
        m_thread = new vector<thread*>(threadSize, nullptr);
        for (auto& it : *m_thread)
            it = new thread(&ThreadPool<T>::process, this);
    }
    catch (std::bad_alloc & e)
    {
        std::cerr << "thread init failed!\n";
        exit(-1);
    }

    INFO(initializer_list<string>({__FILE__, ":ThreadPool initialize completely! Create ", std::to_string(m_threadSize), " threads."}));
    
    return true;
}

template<typename T>
bool ThreadPool<T>::addTask(const shared_ptr<T>& task)
{
    if (m_taskQueue->size() >= m_taskQueueSize)
    {
        m_cond.notify_all();
        return false;
    }
    
    m_taskQueue->emplace(task);
    m_cond.notify_all();
    return true;
}

template<typename T>
shared_ptr<T> ThreadPool<T>::takeTask()
{
    std::unique_lock<mutex> lock(m_mutex);
    
    while (m_taskQueue->empty())
    {
        m_cond.wait(lock);
        if (!m_start)
            return shared_ptr<T>();
    }

    auto ret = m_taskQueue->front();
    m_taskQueue->pop();
    return ret;
}

template<typename T>
void ThreadPool<T>::process()
{
    while (m_start)
    {
        shared_ptr<T> task = takeTask();
        if (task)
            task->run();
    }
}

template<typename T>
void ThreadPool<T>::freeThreadPool()
{
    m_start = false;
    m_cond.notify_all();

    if (m_thread != nullptr)
    {
        for (auto it = m_thread->begin(); it != m_thread->end(); ++it)
        {
            (*it)->join();
            delete (*it);
            (*it) = nullptr;
        }
        
        delete m_thread;
        m_thread = nullptr;
    }

    if (m_taskQueue != nullptr)
        delete m_taskQueue;
    m_taskQueue = nullptr;

    if (g_instance != nullptr)
        delete g_instance;
    g_instance = nullptr;
}

#endif