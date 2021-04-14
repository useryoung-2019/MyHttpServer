/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-07
版本： v1.0
描述： 定时器为一个模拟的时间轮，主要采用list容器，自身不提供node节点的内存申请与释放，需要使用者
      自身申请与释放,每个时间槽的时间顺序为升序
********************************************************************************/
#ifndef TIMER_HPP
#define TIMER_HPP

#include<unistd.h>
#include<ctime>
#include<list>
#include"../log/log_sys.hpp"
#include"timer_node.hpp"

class TimerNode;

class TimeWheel
{
public:
    static const int s_timeSlot = 60;/*时间轮的精度间隔， 设置为60代表精度为1s*/
    TimeWheel();
    ~TimeWheel();

private:
    std::list<TimerNode*>* m_slot[s_timeSlot];/*指针数组，60个含有node的链表指针*/
    time_t m_startTime;/*构造该类的初始时间*/

public:
    /*
    * 函数功能： 向时间轮上添加节点
    * 输入参数： node  TimerNode*
    * 返回值：  成功返回true，失败返回false
    */
    bool addNode(TimerNode* node);

    /*
    * 函数功能： 从时间轮上删除节点
    * 输入参数： node TimerNode*
    * 返回值：  成功返回true，失败返回false
    */
    bool delNode(TimerNode* node);

    /*
    * 函数功能： 修改时间轮上的节点，调整时间参数
    * 输入参数： node TimerNode*
    *          timeout 额外添加/减去的时间
    * 返回值：  成功返回true，失败返回false
    */
    bool modNode(TimerNode* node, time_t timeout);

    /*
    * 函数功能： 时间轮上删去满足条件的节点
    * 输入参数： void
    * 返回值：  void
    */
    void tick();
};

#endif