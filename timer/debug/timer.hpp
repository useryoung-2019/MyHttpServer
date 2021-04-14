#ifndef TIMER_HPP
#define TIMER_HPP

#include<unistd.h>
#include<ctime>
#include<list>
#include<cstdlib>
#include"timer_node.hpp"


class TimeWheel
{
public:
    static const int s_timeSlot = 10;
    TimeWheel();
    ~TimeWheel();

private:
    std::list<TimerNode*>* m_slot[s_timeSlot];
    time_t m_startTime;

public:
    bool addNode(TimerNode* node);
    bool delNode(TimerNode* node);
    bool modNode(TimerNode* node, time_t timeout);
    void tick();

    void show();
};

#endif