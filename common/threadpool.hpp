#pragma once

// 线程池基于 https://github.com/progschj/ThreadPool/tree/master 设计

// TODO 设计线程池 
// 功能：
// 1. 基础入队、出队
// 2. 动态调整


#include <iostream>
#include <thread>
#include <future>
#include <atomic>
#include <list>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    static ThreadPool* getInstance(size_t n = 0);   //单例模式实现

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) //将任务推入队列
    -> std::future<typename std::invoke_result_t<F, Args...>>;	//c++17 invoke_result_t用于获取推导的函数返回值

    template<class F>   // 无参函数版本
    auto enqueue(F&& f)
    -> std::future<typename std::invoke_result_t<F>>;

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;	//防止拷贝
    ThreadPool& operator=(const ThreadPool&) = delete;
private:
    ThreadPool(size_t);

private:
    static ThreadPool* _instance;
    static std::once_flag _flag;   //用于初始化函数

    std::vector< std::thread > _workers;    // 工作线程
    std::queue< std::function<void()> > _tasks; // 任务队列
    std::mutex _queue_mutex;    // 用于保护任务队列的互斥锁
    std::condition_variable _condition; // 用于同步操作
    std::atomic<bool> _stop = false;
};

std::once_flag ThreadPool::_flag;	//静态成员要在类外初始化
ThreadPool* ThreadPool::_instance = nullptr;

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) //将任务推入队列
-> std::future<std::invoke_result_t<F, Args...>>	//返回一个期待，用于获取函数运行结果/异常结果
{
    using result_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared< std::packaged_task<result_type()> > (
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );  // 包装任务成为异步任务。使用shared_ptr来动态管理存亡。

    std::future<result_type> res = task->get_future();  //期待绑定task
    {
        std::lock_guard<std::mutex> lock(_queue_mutex); // 保护任务队列，
        if(_stop)   throw std::runtime_error("在停止的线程池中加入任务");
        _tasks.emplace([task] { (*task)(); });  //再次将任务包装成 function<void()>。
    }
    _condition.notify_one();    //通知线程有任务可以消费了
    return res;
}


template<class F>
auto ThreadPool::enqueue(F&& f)
-> std::future<typename std::invoke_result_t<F>> {
    using result_type = std::invoke_result_t<F>;

    auto task = std::make_shared<std::packaged_task<result_type()>> (
            std::forward<F>(f)
    );  // 包装任务成为异步任务。使用shared_ptr来动态管理存亡。

    std::future<result_type> res = task->get_future();  //期待绑定task
    {
        std::lock_guard<std::mutex> lock(_queue_mutex); // 保护任务队列，
        if(_stop)   throw std::runtime_error("在停止的线程池中加入任务");
        _tasks.emplace([task] { (*task)(); });  //再次将任务包装成 function<void()>。
    }
    _condition.notify_one();    //通知线程有任务可以消费了
    return res;
}

ThreadPool* ThreadPool::getInstance(size_t n)   //单例模式实现
{   //c++11 的call_once，可用于保证线程安全地初始化单例对象，从而杜绝双重检查语句。
    std::call_once(_flag, [n] { _instance = new ThreadPool(n); });
    return _instance;
}

ThreadPool::ThreadPool(size_t n)
{
    for(int i = 0; i < n; ++i)
    {
        _workers.emplace_back
        (
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(_queue_mutex);    //保护任务队列
                        //当队列为空时，等待新任务
                        _condition.wait(lock, [this]{ return _stop || !_tasks.empty(); });
                        if(_stop && _tasks.empty()) return; // 线程池停止，且没有任务，则退出
                        task = std::move(_tasks.front()); // 从队列获取任务
                        _tasks.pop();
                    }
                    task(); // 并发执行任务
                }
            }
        );
    }
}

ThreadPool::~ThreadPool()
{
    _stop.store(true);	
    _condition.notify_all();
    for(auto& it : _workers)	//回收线程资源
        it.join(); 	
}
