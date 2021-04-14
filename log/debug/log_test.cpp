#include"../log_sys.hpp"
#include<iostream>
#include<random>
#include<ctime>
using namespace std;

int main()
{
    LogSys::getInstance()->initLogSys(10);//同步测试
    //LogSys::getInstance()->initLogSys(10, true, 2000);//异步测试

    ifstream in("str.txt");

    string str[50];
    
    for(int i = 0; i < 50; ++i)
    {
        getline(in, str[i]);
    }

    std::default_random_engine random(time(NULL));
    std::uniform_int_distribution<unsigned int> dis1(1, 4);
    for(int i = 0; i < 50; ++i)
    {
        int temp = dis1(random);

        switch (temp)
        {
            case 1:
            {
                DEBUG(std::initializer_list<string>({str[i], "memeda"}));
                break;
            }
            case 2:
            {
                INFO({str[i]});
                break;
            }
            case 3:
            {
                WARN({str[i]});
                break;
            }
            case 4:
            {
                ERROR({str[i]});
                break;
            }
        }
    }

    //LogSystem::getInstance()->loop();

    in.close();
    return 0;
}