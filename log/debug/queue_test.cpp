#include"../block_queue.hpp"
#include<iostream>
#include<fstream>
#include<thread>
using namespace std;

ifstream file;
BlockQueue<string> s;

void cb()
{
    string str;
    while(true)
    {
        s.takeTask(str);
        cout << str << endl;
        s.delTask();
    }
}


int main()
{
    file.open("str.txt");
    string str[50];

    for(int i = 0; i < 50; ++i)
    {
        getline(file, str[i]);
        s.addTask(str[i]);
    }
    
    thread th(&cb);
    th.join();

    return 0;
}