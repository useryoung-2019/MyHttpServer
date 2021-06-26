#include"Timer.hpp"

Timer::Timer()
{
    m_wheel = std::make_shared<vector<List>>(g_slot, List());
    INFO(LOG({__FILE__, ": create ", std::to_string(g_slot), " slots in Timer"}));

    time(&m_startTime);
}

bool Timer::addNode(pTimerNode& node)
{
    if (!node)
        return false;
    
    int slot = static_cast<int> (node->m_timeout % g_slot);
    node->m_slot = slot;

    if ((*m_wheel)[slot].size() == 0 || (*m_wheel)[slot].back()->m_timeout <= node->m_timeout)
    {
        (*m_wheel)[slot].push_back(node);
    }
    else
    {
        auto it = (*m_wheel)[slot].begin();
        for (; it != (*m_wheel)[slot].end(); ++it)
        {
            if (node->m_timeout <= (*it)->m_timeout)
                break;
        }

        (*m_wheel)[slot].insert(it, node);
    }

    INFO(LOG({__FILE__, ": add node to timer successfully, sockfd = ", std::to_string(node->getSockfd())}));
    return true;
}

bool Timer::delNode(pTimerNode& node)
{
    if (!node)
        return false;
    
    (*m_wheel)[node->m_slot].remove(node);
    return true;
}

bool Timer::modNode(pTimerNode& node, time_t extraTime)
{
    if (delNode(node))
    {
        node->m_timeout += extraTime;
        return addNode(node);
    }
    return false;
}

void Timer::tick()
{
    time(&m_startTime);

    for (auto& i : *m_wheel)
    {
        auto iter = i.begin();
        while (iter != i.end())
        {
            if ((*iter)->m_timeout <= m_startTime)
            {
                (*iter)->expireHandle();
                iter = i.erase(iter);
            }
            else
            {
                break;
            }
        }
    }
}

void Timer::destoryTimer()
{
    for (auto& it : *m_wheel)
    {
        it.clear();
    }
}

Timer::~Timer()
{
    destoryTimer();
}