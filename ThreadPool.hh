#pragma once
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <queue>

class ThreadPool {
    public:
        ThreadPool(size_t n_threads = std::thread::hardware_concurrency());
        ~ThreadPool(); 

        std::future<void>  submit_task(std::function<void()> task);

    private:
        void thread_loop();

        std::vector<std::thread> threads;
        std::queue<std::packaged_task<void()>> tasks;

        std::mutex tasks_queue_mutex;
        std::condition_variable cv;

        bool stop;

};