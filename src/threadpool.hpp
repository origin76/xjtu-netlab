#pragma once
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    template <typename F>
    void enqueue(F&& job);

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};

ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([this] {
            while (true) {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { return stop || !jobs.empty(); });
                    if (stop && jobs.empty()) return;
                    job = std::move(jobs.front());
                    jobs.pop();
                }
                job();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& thread : threads) {
        thread.join();
    }
}

template <typename F>
void ThreadPool::enqueue(F&& job) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
        jobs.emplace([job = std::forward<F>(job)] { job(); });
    }
    condition.notify_one();
}