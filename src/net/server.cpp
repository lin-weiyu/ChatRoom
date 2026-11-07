#include "chatroom/net/server.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <vector>

namespace chatroom::net {

Server::Server(std::uint16_t port, int backlog) : acceptor_(port, backlog) {
    if (acceptor_.valid()) {
        poller_.add(acceptor_.fd(), EPOLLIN);
    }
}

Server::~Server() = default;

void Server::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void Server::on_new_connection(Acceptor::Accepted&& accepted) {
    const int fd = accepted.fd.get();

    auto conn = std::make_unique<Connection>(std::move(accepted.fd), std::move(accepted.peer));
    conn->set_handlers(message_handler_, [this](Connection& c) {
        // 回调里不能直接修改 conns_（会在遍历中失效），只做标记，
        // 真正的移除交给事件循环末尾的关闭逻辑。
        c.shutdown();
        update_interest(c);
    });

    // EPOLLRDHUP：对端 shutdown 写方向时通知我们。
    // 没有它，半关闭连接要等到下一次 recv 才发现。
    poller_.add(fd, EPOLLIN | EPOLLRDHUP);
    conns_.emplace(fd, std::move(conn));
}

void Server::update_interest(Connection& conn) {
    std::uint32_t events = EPOLLIN | EPOLLRDHUP;
    if (conn.want_write()) {
        // 只有真的有积压才注册 EPOLLOUT。常驻注册会让 epoll 在每次
        // 可写时都唤醒我们，白白烧 CPU。
        events |= EPOLLOUT;
    }
    poller_.mod(conn.fd(), events);
}

void Server::close_connection(int fd) {
    auto it = conns_.find(fd);
    if (it == conns_.end()) {
        return;
    }
    // 顺序很重要：先从 epoll 摘掉，再让 UniqueFd 析构 close。
    // 反过来做（先 close 再 DEL）会拿一个已关闭的 fd 去调 epoll_ctl，
    // 必然 EBADF——旧代码犯的就是这个错。
    poller_.del(fd);
    conns_.erase(it);
}

void Server::handle_event(int fd, std::uint32_t events) {
    if (fd == acceptor_.fd()) {
        if ((events & EPOLLIN) != 0) {
            acceptor_.accept_all([this](Acceptor::Accepted&& accepted) {
                on_new_connection(std::move(accepted));
            });
        }
        return;
    }

    auto it = conns_.find(fd);
    if (it == conns_.end()) {
        return;
    }

    Connection& conn = *it->second;

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

    update_interest(conn);
}

void Server::run() {
    running_ = true;

    while (running_) {
        run_once();
    }
}

void Server::run_once(int timeout_ms) {
    // 超时不是 0：留出返回控制权的机会，让调用方能检查退出标志。
    const int n = poller_.wait(timeout_ms);

    if (n <= 0) {
        return;
    }

    for (int i = 0; i < n; ++i) {
        const epoll_event& ev = poller_.events()[i];
        handle_event(ev.data.fd, ev.events);
    }
}

}  // namespace chatroom::net
