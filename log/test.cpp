#include"LogQueue.hpp"
#include"Log.hpp"
#include<thread>
#include<vector>
#include<chrono>
#include<fstream>
using namespace std;

int main()
{
    ifstream file("/home/young/VSCODE/local/user.txt");
    Log::getInstance()->initLog(5000, true, 2000);

    while (!file.eof())
    {
        string str;
        getline(file, str);
        DEBUG({str});
    }
    this_thread::sleep_for(chrono::seconds(2));
    Log::getInstance()->freeLog();
    return 0;
}

/*
#include"LogQueue.hpp"
#include<iostream>
#include<fstream>
using namespace std;

LogQueue<string> que;

int main()
{
    ifstream file("/home/young/VSCODE/local/user.txt");
    while (file.eof())
    {
        string str;
        getline(file, str);
        que.addTask(make_shared<string>(str));
    }

        

}*/