// thread_pool.h — 固定大小的线程池
//
// 所属层：infra（平台无关）
// 被谁使用：app::ChatServer
//
// 为什么不用"一个任务开一个线程"：线程创建约 10-50 微秒，默认栈 8MB。
// 一万个任务就是一万个线程，虚拟内存直接爆掉。池化把创建成本摊薄，
// 也让线程数可控。

#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace chatroom::infra {

/// 固定 N 个 worker 的线程池。
///
/// 生命周期契约：析构会等待队列里**所有已提交**的任务执行完再返回，
/// 不会半路丢弃任务。
class ThreadPool {
public:
    /// 创建 thread_count 个 worker。传 0 会被当作 1。
    explicit ThreadPool(std::size_t thread_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// 投递任务。线程安全，任何线程都能调用。
    /// 池已停止时静默丢弃（此时任务已经没有执行机会）。
    void submit(std::function<void()> task);

    std::size_t thread_count() const noexcept {
        return threads_.size();
    }

    /// 已排队但还没开始执行的任务数（观测/测试用）。
    std::size_t pending() const;

private:
    /// worker 主循环：等任务 → 取出 → 在锁外执行。
    void worker_loop();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
    bool stopping_ = false;
};

}  // namespace chatroom::infra
