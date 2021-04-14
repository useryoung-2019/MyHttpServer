#include"../connectpool.hpp"
#include<iostream>
#include<unistd.h>
using namespace std;

void fun(ConnectPool* p)
{
    MYSQL* mysql;
    p->getMysql(mysql);
    string ss = "SELECT * FROM orders";

    mysql_real_query(mysql, ss.c_str(), ss.length());
    MYSQL_RES* res;
    MYSQL_ROW row;

    res = mysql_use_result(mysql);
    while((row = mysql_fetch_row(res)) != NULL)
    {
        unsigned num = mysql_num_fields(res);
        for(int i = 0; i < num; ++i)
        {
            cout << row[i] << " ";
        }

        cout << endl;
    }
}

int main()
{
    ConnectPool* po = ConnectPool::getInstance();


    //cout << me.m_host << endl;

    po->initPool();
    fun(po);

    //sleep(10);
    return 0;
}