#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chatroom/chat/chat_hub.h"
#include "chatroom/net/server.h"

namespace chatroom::app {

// 装配层：把网络层的字节流和业务层的帧语义接起来。
//
// 它是整个项目里唯一同时认识 net::Connection 和 chat::ChatHub 的地方。
// 这个"唯一"是刻意的：分层要求跨层的地方越少越好，最好只有一处。
class ChatServer {
public:
    explicit ChatServer(std::uint16_t port);

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    bool valid() const noexcept { return server_.valid(); }

    std::uint16_t port() const noexcept { return server_.port(); }

    std::size_t session_count() const noexcept { return hub_.session_count(); }

    // 处理一轮事件，返回处理后的会话数（测试里用来断言）。
    void run_once(int timeout_ms = 200);

private:
    void on_open(net::Connection& conn);
    void on_message(net::Connection& conn, std::string_view data);
    void on_close(net::Connection& conn);

    void dispatch(const std::vector<chat::Outbound>& outbounds);

    net::Server server_;
    chat::ChatHub hub_;

    // fd 与 session 的双向映射：
    // 网络层按 fd 找连接，业务层按 session 找用户，两张表在 app 层对齐。
    std::unordered_map<int, chat::SessionId> fd_to_session_;
    std::unordered_map<chat::SessionId, net::Connection*> session_to_conn_;
};

}  // namespace chatroom::app
