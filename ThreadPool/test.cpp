#include<iostream>
#include"ThreadPool.hpp"
using namespace std;

struct Test
{
    void run()
    {
        cout << "hello!\n";
    }
};


int main()
{   
    Log::getInstance()->initLog(200, true, 200);
    
    auto pool = ThreadPool<Test>::getInstance();
    pool->init();

    
    this_thread::sleep_for(chrono::seconds(2));
    Log::getInstance()->freeLog();
    pool->freeThreadPool();

    return 0;
}

