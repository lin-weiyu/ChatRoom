#pragma once

#include "chatroom/app/chat_server.h"
#include "chatroom/infra/config.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

namespace chatroom::testing {

// 在后台线程里跑一个真实的 ChatServer。
//
// 端口传 0 让内核分配，避免测试之间抢端口（这也是为什么 server 要暴露
// 真实的监听端口）。
class TestServer {
public:
    explicit TestServer(int worker_threads = 0, int idle_timeout_seconds = 0) {
        config_.port = 0;
        config_.worker_threads = worker_threads;
        config_.idle_timeout_seconds = idle_timeout_seconds;
        config_.verbose = false;

        server_ = std::make_unique<app::ChatServer>(config_);
        if (server_->valid()) {
            thread_ = std::thread([this] {
                while (!stop_.load(std::memory_order_relaxed)) {
                    server_->run_once(5);
                }
            });
        }
    }

    ~TestServer() {
        stop_.store(true, std::memory_order_relaxed);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    TestServer(const TestServer&) = delete;
    TestServer& operator=(const TestServer&) = delete;

    bool valid() const {
        return server_ != nullptr && server_->valid();
    }

    std::uint16_t port() const {
        return server_->port();
    }

private:
    infra::Config config_;
    std::unique_ptr<app::ChatServer> server_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
};

}  // namespace chatroom::testing
