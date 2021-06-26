#include"server.hpp"
#include<iostream>
using namespace std;

int main(int argc, char** argv)
{
    shared_ptr<Server> server(new Server());
    server->getPara(argc, argv);
    server->initLog();
    server->init();
    server->setPipeSig();
    server->initSQL();
    server->initThreadPool();

    server->Listen();
    server->Epoll();
    server->Loop();
    server->destoryServer();

    return 0;
}