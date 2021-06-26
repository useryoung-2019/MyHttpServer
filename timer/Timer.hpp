#ifndef TIMER_H
#define TIMER_H

#include"../log/Log.hpp"
#include"TimerNode.hpp"
#include<list>
#include<vector>

using std::vector;
using std::list;

class Timer
{
public:
    typedef shared_ptr<TimerNode> pTimerNode;
    typedef list<pTimerNode> List;
    const int g_slot = 60;

public:
    Timer();
    ~Timer();

    bool addNode(pTimerNode& node);
    bool delNode(pTimerNode& node);
    bool modNode(pTimerNode& node, time_t extraTime);
    void tick();
    

private:
    shared_ptr<vector<List>> m_wheel;
    time_t m_startTime;
    
    void destoryTimer();
};

#endif