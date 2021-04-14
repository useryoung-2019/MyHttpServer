#include"timer.hpp"
#include<iostream>
#include<cstring>
#include<signal.h>
using namespace std;

bool pan = false;

void fun(int num)
{
    if(num == SIGALRM)
    {
        pan = true;
        alarm(3);
    }
    
}


int main()
{
    TimerNode node[60];
    TimeWheel wheel;

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = &fun;
    sigfillset(&sa.sa_mask);
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    for(int i =0; i < 60; ++i)
    {
        node[i].m_timeout = time(&node[i].m_timeout);
        wheel.addNode(&node[i]);
        sleep(1);
    }
    wheel.show();
    wheel.modNode(&node[13], 5);

    wheel.show();
    wheel.tick();
    //alarm(3);
    wheel.show();
    wheel.delNode(&node[35]);
    wheel.show();
    /*while(true)
    {
        if(pan)
        {
            wheel.tick1(3);
            wheel.show();
            pan = false;
        }
    }*/

    return 0;
}