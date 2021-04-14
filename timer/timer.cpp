#include"timer.hpp"

TimeWheel::TimeWheel()
{
    for(int i = 0; i < s_timeSlot; ++i)
    {
        m_slot[i] = new std::list<TimerNode*>;
        
        if (m_slot[i] == NULL)
        {
            ERROR(std::initializer_list<std::string>({__FILE__, ":", 
                    std::to_string(__LINE__), "->  fail to new object"}));
            exit(-1);
        }
    }
    INFO(std::initializer_list<std::string>({__FILE__, ":", std::to_string(__LINE__), 
        "-> create ", std::to_string(s_timeSlot), " slots in TimeWheel"}));
    time(&m_startTime);/*构造对象即初始化构造事件*/
}

TimeWheel::~TimeWheel()
{
    for(int i = 0; i < s_timeSlot; ++i)
    {
        if (m_slot[i] != NULL)
        {
            delete m_slot[i];
        }
    }
}

bool TimeWheel::addNode(TimerNode* node)
{
    if (node == NULL || node->m_timeout < m_startTime)
    {
        return false;
    }

    int slot = static_cast<int> ((node->m_timeout - m_startTime) % s_timeSlot);
    node->m_slot = slot;

    if ((m_slot[slot]->size() == 0) || (m_slot[slot]->back()->m_timeout < node->m_timeout))
    {
        m_slot[slot]->push_back(node);
    }
    else
    {
        std::list<TimerNode*>::iterator i = m_slot[slot]->begin();
        TimerNode* temp;
        for(; i != m_slot[slot]->end(); ++i)
        {
            temp = *i;

            if (node->m_timeout > temp->m_timeout)/*升序的必要条件*/
            {
                continue;
            }
            else
            {
                break;
            }
        }
        m_slot[slot]->insert(i, node);
    }
    return true;
}

bool TimeWheel::delNode(TimerNode* node)
{
    if (node == NULL || node->m_timeout < m_startTime)
    {
        return false;
    }

    m_slot[node->m_slot]->remove(node);
    return true;
}

bool TimeWheel::modNode(TimerNode* node, time_t timeout)
{
    if (delNode(node))
    {
        node->m_timeout += timeout;
        return addNode(node);
    }
    return false;
}

/*
* 函数功能： 每次调用都会基于调用时间进行时间轮节点的处理，将过期时间删除
*/
void TimeWheel::tick()
{
    time_t now = time(&now);
    TimerNode* temp = NULL;

    for(int i = 0; i < s_timeSlot; ++i)//每tick一次，时间轮做一次轮询
    {
        auto iter = m_slot[i]->begin();
        
        while(iter != m_slot[i]->end())
        {
            temp = *iter;

            if (temp->m_timeout <= now)
            {
                temp->callbackFunc();
                iter = m_slot[i]->erase(iter);/*erase会导致list的该迭代器失效，
                                                必要的处理*/
            }
            else
            {
                break;
            }
        }
    }
}