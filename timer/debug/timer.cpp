#include"timer.hpp"
#include<iostream>

TimeWheel::TimeWheel()
{
    for(int i = 0; i < s_timeSlot; ++i)
    {
        m_slot[i] = new std::list<TimerNode*>;
        
        if (m_slot[i] == NULL)
        {
            /*ERROR(std::initializer_list<std::string>({__FILE__, ":", __func__,
                  "->  fail to new object"}));*/
            exit(-1);
        }
    }
    time(&m_startTime);
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
    if (node == NULL || node->m_timeout < s_timeSlot)
    {
        return false;
    }

    int slot = static_cast<int> ((node->m_timeout - m_startTime) % s_timeSlot);
    node->m_slot = slot;

    if (m_slot[slot]->size() == 0)
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

            if (node->m_timeout > temp->m_timeout)
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

void TimeWheel::tick()
{
    //time_t now = time(&now);
    time_t now = m_startTime + 26;
    TimerNode* temp = NULL;

    for(int i = 0; i < s_timeSlot; ++i)
    {
        auto iter = m_slot[i]->begin();
        
        while(iter != m_slot[i]->end())
        {
            temp = *iter;

            if (temp->m_timeout <= now)
            {
                temp->callbackFunc();
                iter = m_slot[i]->erase(iter);
            }
            else
            {
                break;
            }
        }
    }
}

void TimeWheel::show()
{
    TimerNode* temp = NULL;
    std::cout << m_startTime << std::endl;
    for(int i = 0; i < s_timeSlot; ++i)
    {
        auto iter = m_slot[i]->begin();
        
        std::cout << i << ">>>  ";
        for(; iter != m_slot[i]->end(); ++iter)
        {
            temp = *iter;
            std::cout << temp->m_timeout << "  ";
        }
        std::cout << std::endl;
    }
}












