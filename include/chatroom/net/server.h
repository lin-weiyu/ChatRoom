#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "chatroom/net/acceptor.h"
#include "chatroom/net/connection.h"
#include "chatroom/net/poller.h"

namespace chatroom::net {

// 把 Poller、Acceptor、Connection 组装成一个能跑的事件循环。
//
// 这一层是"装配"：它知道怎么把 fd 变成 Connection，也知道事件来了该
// 分发给谁。它不知道聊天——M3 之后，业务通过回调注入进来。
class Server {
public:
    explicit Server(std::uint16_t port, int backlog = 128);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // 监听端口（构造时传 0 则返回内核分配的真实端口）
    std::uint16_t port() const noexcept { return acceptor_.port(); }

    bool valid() const noexcept { return acceptor_.valid(); }

    // 收到数据时调用。默认实现是 echo，M1 之后会被协议解析替换。
    using MessageHandler = Connection::MessageHandler;
    void set_message_handler(MessageHandler handler);

    // 跑事件循环，直到 stop() 被调用。
    void run();

    // 只处理一轮事件（最多等 timeout_ms 毫秒）。
    // 拆出这一层是为了让调用方能在两轮之间做别的事——比如检查退出信号。
    void run_once(int timeout_ms = 200);

    void stop() noexcept { running_ = false; }

    std::size_t connection_count() const noexcept { return conns_.size(); }

private:
    void handle_event(int fd, std::uint32_t events);
    void on_new_connection(Acceptor::Accepted&& accepted);
    void update_interest(Connection& conn);
    void close_connection(int fd);

    Poller poller_;
    Acceptor acceptor_;
    std::unordered_map<int, std::unique_ptr<Connection>> conns_;
    MessageHandler message_handler_;
    bool running_ = false;
};

}  // namespace chatroom::net
