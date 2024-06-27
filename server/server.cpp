//
// Created by lang liu on 2024/6/26.
//

#include "broker.hpp"

int main()
{
    using namespace MyMQ;
    system("pwd");

    Server server(8888, "./data/");
    server.Start();

    return 0;
}