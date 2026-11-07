// server.cpp — 事件循环实现
//
// 整个服务器的主干就在这里：
//
//   epoll_wait 返回一批就绪的 fd
//        ↓
//   监听 fd  → accept 新连接
//   客户端 fd → 读 / 写
//        ↓
//   调整 EPOLLOUT 关注（有积压才加）
//
// 注意所有回调都可能在"连接已经不可用"的状态下被触发，所以每次操作之后
// 都要重新检查 conn.closed()。

#include "chatroom/net/server.h"

#include <cerrno>
#include <sys/epoll.h>
#include <vector>

#include <unistd.h>

namespace chatroom::net {

Server::Server(std::uint16_t port, int backlog) : acceptor_(port, backlog) {
    if (acceptor_.valid()) {
        // 先把监听 fd 注册进 epoll：只有它可读，才说明有新连接到来。
        poller_.add(acceptor_.fd(), EPOLLIN);
    }
}

Server::~Server() = default;

void Server::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void Server::set_open_handler(OpenHandler handler) {
    open_handler_ = std::move(handler);
}

void Server::set_close_handler(CloseHandler handler) {
    close_handler_ = std::move(handler);
}

void Server::set_wakeup_fd(int fd, std::function<void()> handler) {
    wakeup_fd_ = fd;
    wakeup_handler_ = std::move(handler);
    if (fd >= 0) {
        poller_.add(fd, EPOLLIN);
    }
}

void Server::on_new_connection(Acceptor::Accepted&& accepted) {
    const int fd = accepted.fd.get();

    auto conn = std::make_unique<Connection>(std::move(accepted.fd), std::move(accepted.peer));

    // 回调捕获 this 是安全的：Connection 的生命周期不超过 Server。
    conn->set_message_handler([this](Connection& c, std::string_view data) {
        if (message_handler_) {
            message_handler_(c, data);
        }
    });

    // EPOLLRDHUP：对端 shutdown 写方向时通知我们。
    // 没有它，半关闭的连接要等到下一次 recv 才发现。
    poller_.add(fd, EPOLLIN | EPOLLRDHUP);

    // 先取出裸指针再 move：move 之后 conn 就空了。
    Connection* raw = conn.get();
    conns_.emplace(fd, std::move(conn));

    if (open_handler_) {
        open_handler_(*raw);
    }
}

void Server::update_interest(Connection& conn) {
    std::uint32_t events = EPOLLIN | EPOLLRDHUP;
    if (conn.want_write()) {
        // 只有真的有积压才注册 EPOLLOUT。常驻注册会让 epoll 在每次可写时
        // 都唤醒我们——而 socket 绝大多数时间都是可写的，等于空转烧 CPU。
        events |= EPOLLOUT;
    }
    poller_.mod(conn.fd(), events);
}

void Server::close_connection(int fd) {
    auto it = conns_.find(fd);
    if (it == conns_.end()) {
        return;
    }

    // 顺序很重要：先通知业务层清理状态，再从 epoll 摘除，最后才让
    // UniqueFd 析构去 close。
    //
    // 反过来做（先 close 再 epoll_ctl(DEL)）会拿一个已关闭的 fd 去调
    // epoll_ctl，必然 EBADF——早期版本犯的就是这个错。
    if (close_handler_) {
        close_handler_(*it->second);
    }
    poller_.del(fd);
    conns_.erase(it);
}

void Server::handle_event(int fd, std::uint32_t events) {
    // 叫醒 fd 不是业务连接，单独处理。
    if (fd == wakeup_fd_) {
        if (wakeup_handler_) {
            wakeup_handler_();
        }
        return;
    }

    if (fd == acceptor_.fd()) {
        if ((events & EPOLLIN) != 0) {
            acceptor_.accept_all(
                [this](Acceptor::Accepted&& accepted) { on_new_connection(std::move(accepted)); });
        }
        return;
    }

    auto it = conns_.find(fd);
    if (it == conns_.end()) {
        // 连接可能已经被上一轮事件关闭，而 epoll 这次还把它报出来了。
        // 这不是 bug，只是"事件和状态之间有时间差"。
        return;
    }

    Connection& conn = *it->second;

    // EPOLLERR/EPOLLHUP 表示连接已经出问题，不必再等 EPOLLIN。
    if ((events & (EPOLLERR | EPOLLHUP)) != 0) {
        conn.shutdown();
    }

    if ((events & EPOLLIN) != 0) {
        conn.handle_read();
    }

    if ((events & EPOLLOUT) != 0) {
        conn.handle_write();
    }

    if (conn.closed()) {
        close_connection(fd);
        return;
    }

    // 读写可能改变了积压状态（比如刚把缓冲发完），要重新调整关注的事件。
    update_interest(conn);
}

void Server::run() {
    running_ = true;
    while (running_) {
        run_once();
    }
}

void Server::run_once(int timeout_ms) {
    // 超时不是 -1：每隔一段时间返回一次，让调用方有机会检查退出标志。
    const int n = poller_.wait(timeout_ms);

    if (n <= 0) {
        return;  // 超时或被信号打断，都不是错误
    }

    for (int i = 0; i < n; ++i) {
        const epoll_event& ev = poller_.events()[i];
        handle_event(ev.data.fd, ev.events);
    }
}

}  // namespace chatroom::net
