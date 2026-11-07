#include "chatroom/infra/thread_pool.h"

#include <utility>

namespace chatroom::infra {

ThreadPool::ThreadPool(std::size_t thread_count) {
    if (thread_count == 0) {
        thread_count = 1;
    }
    threads_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();

    for (std::thread& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

std::size_t ThreadPool::pending() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::worker_loop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 用谓词等待，防止虚假唤醒（spurious wakeup）导致空转
            cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

            if (stopping_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }
        // 在锁外执行：任务内部可能再次加锁或耗时很久
        task();
    }
}

}  // namespace chatroom::infra
