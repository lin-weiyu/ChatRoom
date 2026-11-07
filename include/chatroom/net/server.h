// server.h — 事件循环：把 Poller / Acceptor / Connection 装配起来
//
// 所属层：net
// 被谁使用：app::ChatServer（通过回调注入业务逻辑）
//
// 这一层是"调度中心"：它知道怎么把 fd 变成 Connection，也知道事件来了
// 该分发给谁。它不知道聊天——业务通过三个回调注入：
//   on_open     新连接建立
//   on_message  收到字节
//   on_close    连接关闭（业务必须在这里清理自己的状态）

#pragma once

#include "chatroom/net/acceptor.h"
#include "chatroom/net/connection.h"
#include "chatroom/net/poller.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace chatroom::net {

/// 单线程 epoll 事件循环。
class Server {
public:
    using MessageHandler = Connection::MessageHandler;
    using OpenHandler = std::function<void(Connection&)>;
    using CloseHandler = std::function<void(Connection&)>;

    /// 绑定端口并初始化 epoll。失败时 valid() 返回 false。
    explicit Server(std::uint16_t port, int backlog = 128);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// 实际监听端口（构造时传 0 则返回内核分配的真实端口）。
    std::uint16_t port() const noexcept {
        return acceptor_.port();
    }

    bool valid() const noexcept {
        return acceptor_.valid();
    }

    void set_message_handler(MessageHandler handler);
    void set_open_handler(OpenHandler handler);
    void set_close_handler(CloseHandler handler);

    /// 注册一个"叫醒 fd"（通常是 eventfd）。
    ///
    /// 工作线程有结果要交给 IO 线程时，eventfd 变可读 → epoll_wait 立刻
    /// 返回 → 调用 handler。没有它，IO 线程只能靠短超时轮询，白白浪费 CPU。
    void set_wakeup_fd(int fd, std::function<void()> handler);

    /// 跑事件循环直到 stop() 被调用。内部就是反复调用 run_once()。
    void run();

    /// 只处理一轮事件（最多等 timeout_ms 毫秒）。
    ///
    /// 拆出这一层是为了让调用方能在两轮之间做别的事——比如检查退出信号。
    void run_once(int timeout_ms = 200);

    void stop() noexcept {
        running_ = false;
    }

    std::size_t connection_count() const noexcept {
        return conns_.size();
    }

private:
    /// 分发一个就绪事件：监听 fd → accept；客户端 fd → 读写。
    void handle_event(int fd, std::uint32_t events);

    /// 接受一个新连接，创建 Connection 并注册到 epoll。
    void on_new_connection(Acceptor::Accepted&& accepted);

    /// 根据"是否有待发送数据"调整 EPOLLOUT 的关注。
    void update_interest(Connection& conn);

    /// 从 epoll 摘除并销毁连接。顺序：先 DEL 再 close。
    void close_connection(int fd);

    Poller poller_;
    Acceptor acceptor_;
    std::unordered_map<int, std::unique_ptr<Connection>> conns_;

    MessageHandler message_handler_;
    OpenHandler open_handler_;
    CloseHandler close_handler_;

    int wakeup_fd_ = -1;
    std::function<void()> wakeup_handler_;

    bool running_ = false;
};

}  // namespace chatroom::net
