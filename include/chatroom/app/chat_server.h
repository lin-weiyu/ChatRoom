// chat_server.h — 装配层：把字节流和业务帧接起来
//
// 所属层：app（最上层）
// 被谁使用：main.cpp、tests/helpers/test_server.h
//
// 这是整个项目里**唯一**同时认识 net::Connection 和 chat::ChatHub 的地方。
// 这个"唯一"是刻意的：分层要求跨层的地方越少越好，最好只有一处。
//
// 线程模型（--workers N）：
//
//   IO 线程（1 个）        epoll_wait、recv/send、Connection 生死
//         │  投递 Frame（只传值，不传指针）
//         ▼
//   工作线程池（N 个）      ChatHub::handle → Outbound
//         │  结果入队 + eventfd 叫醒
//         ▼
//   IO 线程 drain 结果，翻译成 send
//
// 两条铁律：
//   1. Connection / fd / 映射表只在 IO 线程里碰，工作线程绝不直接写 socket
//   2. ChatHub 只被工作线程访问，用一把 mutex 保护
//
// --workers 0 时退化成单线程，用于对比和排错。

#pragma once

#include "chatroom/chat/chat_hub.h"
#include "chatroom/infra/config.h"
#include "chatroom/infra/thread_pool.h"
#include "chatroom/net/event_fd.h"
#include "chatroom/net/server.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace chatroom::app {

/// 完整的聊天服务器：网络 + 业务 + 装配。非线程安全（内部自己管）。
class ChatServer {
public:
    /// 构造即开始监听（但不会运行事件循环，等 run_once）。
    explicit ChatServer(const infra::Config& config);

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    bool valid() const noexcept {
        return server_.valid();
    }

    std::uint16_t port() const noexcept {
        return server_.port();
    }

    std::size_t session_count() const noexcept;

    /// 处理一轮事件。返回后调用方可以检查退出标志。
    ///
    /// timeout_ms 同时是"空闲超时检查"的节拍：每轮都会扫一遍多久没动静的
    /// 连接。所以它不宜设得太大。
    void run_once(int timeout_ms = 200);

    /// 优雅关闭：通知所有客户端，再关掉写方向。
    /// 调用后应再跑几轮 run_once() 让数据真正发出去。
    void shutdown();

private:
    void on_open(net::Connection& conn);
    void on_message(net::Connection& conn, std::string_view data);
    void on_close(net::Connection& conn);

    /// 工作线程里执行：处理一帧，把结果放进结果队列。
    void handle_task(chat::SessionId id, protocol::Frame frame);

    /// IO 线程里执行：取走结果并发送。
    void drain_results();

    /// 把 Outbound 翻译成 conn->send()。只在 IO 线程调用。
    void dispatch(const std::vector<chat::Outbound>& outbounds);

    /// 断开久无动静的连接。
    void check_idle();

    using Clock = std::chrono::steady_clock;

    infra::Config config_;
    net::Server server_;
    chat::ChatHub hub_;

    /// 保护 hub_。粗粒度锁：这个项目的业务临界区很短，细粒度锁收益不大，
    /// 反而容易死锁。
    mutable std::mutex hub_mutex_;

    mutable std::mutex result_mutex_;
    std::vector<chat::Outbound> results_;

    net::EventFd wakeup_;

    /// pool_ 必须最后声明：析构顺序与声明顺序相反，这样线程池会先 join，
    /// 再销毁它用到的 hub_ 和 mutex。
    std::unique_ptr<infra::ThreadPool> pool_;

    /// fd 与 session 的双向映射（仅 IO 线程访问，不需要加锁）。
    std::unordered_map<int, chat::SessionId> fd_to_session_;
    std::unordered_map<chat::SessionId, net::Connection*> session_to_conn_;

    /// 最后一次活动时间（仅 IO 线程访问）。
    std::unordered_map<chat::SessionId, Clock::time_point> last_active_;
};

}  // namespace chatroom::app
