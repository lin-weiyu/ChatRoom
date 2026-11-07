#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chatroom/chat/chat_hub.h"
#include "chatroom/infra/config.h"
#include "chatroom/infra/event_fd.h"
#include "chatroom/infra/thread_pool.h"
#include "chatroom/net/server.h"

namespace chatroom::app {

// 装配层：把网络层的字节流和业务层的帧语义接起来。
//
// 线程模型（M6）：
//
//   IO 线程（只有一个）        epoll_wait、recv/send、Connection 的生死
//         │  投递 Task
//         ▼
//   工作线程池（N 个）         解析出的帧 -> ChatHub::handle -> Outbound
//         │  结果入队 + eventfd 叫醒
//         ▼
//   IO 线程 drain 结果，翻译成 send
//
// 两条铁律：
//   1. Connection / fd / 映射表只在 IO 线程里碰，工作线程绝不直接写 socket
//   2. ChatHub 只被工作线程访问，用一把 mutex 保护
//
// worker_threads = 0 时退化成单线程，用于对比和排错。
class ChatServer {
public:
    explicit ChatServer(const infra::Config& config);

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    bool valid() const noexcept { return server_.valid(); }

    std::uint16_t port() const noexcept { return server_.port(); }

    std::size_t session_count() const noexcept;

    void run_once(int timeout_ms = 200);

    // 优雅关闭：通知所有客户端，再关掉写方向。
    void shutdown();

private:
    void on_open(net::Connection& conn);
    void on_message(net::Connection& conn, std::string_view data);
    void on_close(net::Connection& conn);

    // 工作线程里执行：处理一条帧，把结果放进结果队列
    void handle_task(chat::SessionId id, protocol::Frame frame);

    // IO 线程里执行：取走结果并发送
    void drain_results();

    void dispatch(const std::vector<chat::Outbound>& outbounds);
    void check_idle();

    using Clock = std::chrono::steady_clock;

    infra::Config config_;
    net::Server server_;
    chat::ChatHub hub_;

    // 保护 hub_。粗粒度锁：这个项目的业务临界区很短，细粒度锁收益不大，
    // 反而容易死锁。
    mutable std::mutex hub_mutex_;

    mutable std::mutex result_mutex_;
    std::vector<chat::Outbound> results_;

    infra::EventFd wakeup_;

    // pool_ 必须最后声明：析构顺序是反的，这样线程池会先 join，
    // 再销毁它用到的 hub_ 和 mutex。
    std::unique_ptr<infra::ThreadPool> pool_;

    // fd 与 session 的双向映射（仅 IO 线程访问，不需要加锁）
    std::unordered_map<int, chat::SessionId> fd_to_session_;
    std::unordered_map<chat::SessionId, net::Connection*> session_to_conn_;

    // 最后一次活动时间（仅 IO 线程访问）
    std::unordered_map<chat::SessionId, Clock::time_point> last_active_;
};

}  // namespace chatroom::app
