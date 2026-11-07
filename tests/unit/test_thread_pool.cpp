#include "chatroom/infra/thread_pool.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using chatroom::infra::ThreadPool;

TEST(ThreadPool, RunsSubmittedTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    // 等任务跑完（最多 2 秒）
    for (int i = 0; i < 2000 && counter.load() < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPool, DestructorDrainsQueuedTasks) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(1);
        for (int i = 0; i < 50; ++i) {
            pool.submit([&counter] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }  // 析构：应等所有已入队的任务执行完再 join
    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPool, ConcurrentSubmitsAreSafe) {
    // 真正跑在 TSan 下才有意义；普通构建下至少验证不崩、不丢任务
    ThreadPool pool(2);
    std::atomic<int> counter{0};

    std::vector<std::thread> submitters;
    for (int t = 0; t < 4; ++t) {
        submitters.emplace_back([&pool, &counter] {
            for (int i = 0; i < 100; ++i) {
                pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    for (auto& t : submitters) {
        t.join();
    }

    for (int i = 0; i < 2000 && counter.load() < 400; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_EQ(counter.load(), 400);
}
