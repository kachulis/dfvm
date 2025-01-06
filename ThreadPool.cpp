#include "ThreadPool.hh"
#include <iostream>


ThreadPool::ThreadPool(size_t n_threads) : stop(false) 
{
    for (size_t i = 0; i < n_threads; i++) {
        threads.emplace_back([this](){thread_loop();});
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(tasks_queue_mutex);
        stop = true;
    }
    cv.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }
}

void ThreadPool::thread_loop() {
    while (true) {
        std::unique_lock<std::mutex> lock(tasks_queue_mutex);
        cv.wait(lock, [this] {return !tasks.empty() || stop;});
        if (stop && tasks.empty()) {
            return;
        }
        std::packaged_task<void()> task = std::move(tasks.front());
        tasks.pop();
        lock.unlock();

        task();
    }
}

std::future<void> ThreadPool::submit_task(std::function<void()> task) {
    std::packaged_task<void()> task_p(task);
    std::future<void> result = task_p.get_future();;
    {
        std::unique_lock<std::mutex> lock(tasks_queue_mutex);
        tasks.emplace(std::move(task_p));
    }
    cv.notify_one();

    return result;
}