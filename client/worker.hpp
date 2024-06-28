//
// Created by lang liu on 2024/6/27.
//

#ifndef WORKER_HPP
#define WORKER_HPP

#include "threadpool.hpp"
#include <muduo/net/EventLoopThread.h>

namespace MyMQ {
    class AsyncWorker;

    using AsyncWorkerPtr = std::shared_ptr<AsyncWorker>;

    class AsyncWorker {
    public:
        muduo::net::EventLoopThread _loop;
        ThreadPool* _pool = ThreadPool::getInstance(5);
    };
}

#endif //WORKER_HPP
