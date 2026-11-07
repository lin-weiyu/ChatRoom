#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chatroom/chat/chat_hub.h"
#include "chatroom/infra/config.h"
#include "chatroom/net/server.h"

namespace chatroom::app {

// 装配层：把网络层的字节流和业务层的帧语义接起来。
//
// 它是整个项目里唯一同时认识 net::Connection 和 chat::ChatHub 的地方。
// 这个"唯一"是刻意的：分层要求跨层的地方越少越好，最好只有一处。
class ChatServer {
public:
    explicit ChatServer(const infra::Config& config);

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    bool valid() const noexcept { return server_.valid(); }

    std::uint16_t port() const noexcept { return server_.port(); }

    std::size_t session_count() const noexcept { return hub_.session_count(); }

    // 处理一轮事件。timeout_ms 决定这一轮最多阻塞多久，
    // 顺带作为"检查空闲超时"的节拍。
    void run_once(int timeout_ms = 200);

    // 优雅关闭：通知所有客户端，再关掉写方向。
    void shutdown();

private:
    void on_open(net::Connection& conn);
    void on_message(net::Connection& conn, std::string_view data);
    void on_close(net::Connection& conn);

    void dispatch(const std::vector<chat::Outbound>& outbounds);
    void check_idle();

    using Clock = std::chrono::steady_clock;

    infra::Config config_;
    net::Server server_;
    chat::ChatHub hub_;

    // fd 与 session 的双向映射：
    // 网络层按 fd 找连接，业务层按 session 找用户，两张表在 app 层对齐。
    std::unordered_map<int, chat::SessionId> fd_to_session_;
    std::unordered_map<chat::SessionId, net::Connection*> session_to_conn_;

    // 最后一次活动时间，用于空闲超时。用 steady_clock：系统时间被调整
    // （NTP 校时、手动改时间）时 monotonic 时钟不会跳。
    std::unordered_map<chat::SessionId, Clock::time_point> last_active_;
};

}  // namespace chatroom::app
