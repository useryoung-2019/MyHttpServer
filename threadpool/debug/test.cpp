#include"../threadpool.hpp"
#include<iostream>
using namespace std;

struct task
{
    void run()
    {
        cout << this_thread::get_id() << " run the task" << endl;
    }
};

int main()
{
    ThreadPool<task*> pool;
    task* t[10];

    for(int i = 0; i < 10; ++i)
    {
        pool.addTask(t[i]);
    }

    getchar();

    return 0;
}












