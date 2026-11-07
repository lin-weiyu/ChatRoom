#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace chatroom::infra {

// 固定大小的线程池。
//
// 关键设计：
//   - submit() 是线程安全的，任何线程都能投任务
//   - 任务队列用 mutex + condition_variable，不用忙等
//   - 析构时置停止标志、通知所有线程、join；不在析构里丢任务
//
// 为什么不每个任务开一个线程：线程创建约 10-50 微秒，栈默认 8MB。
// 一万个任务就是一万个线程，内存直接爆掉。池化把"创建"的成本摊薄。
class ThreadPool {
public:
    explicit ThreadPool(std::size_t thread_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task);

    std::size_t thread_count() const noexcept {
        return threads_.size();
    }

    // 仅用于测试/观测：已排队但还没开始的任务数
    std::size_t pending() const;

private:
    void worker_loop();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
    bool stopping_ = false;
};

}  // namespace chatroom::infra
