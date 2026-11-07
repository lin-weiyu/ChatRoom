#include "chatroom/net/connection.h"

#include <cerrno>
#include <sys/socket.h>
#include <utility>

#include <unistd.h>

namespace chatroom::net {

Connection::Connection(platform::UniqueFd fd, std::string peer)
    : fd_(std::move(fd)), peer_(std::move(peer)) {}

void Connection::set_message_handler(MessageHandler on_message) {
    on_message_ = std::move(on_message);
}

bool Connection::handle_read() {
    if (closed_) {
        return false;
    }

    // 一次事件尽量把内核缓冲读空，而不是每次只读 1 字节就返回事件循环。
    // 循环的终止条件是 EAGAIN——这正是"非阻塞"的意义。
    char buf[65536];
    for (;;) {
        const ssize_t n = ::recv(fd_.get(), buf, sizeof(buf), 0);

        if (n > 0) {
            in_.append(buf, static_cast<std::size_t>(n));
            if (on_message_) {
                on_message_(*this, in_.view());
            }
            continue;
        }

        if (n == 0) {
            // 返回 0 表示对端有序关闭（发了 FIN）。这是正常结束，不是错误。
            closed_ = true;
            return false;
        }

        if (errno == EINTR) {
            continue;  // 被信号打断，重试
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;  // 内核缓冲读空了
        }

        closed_ = true;
        return false;
    }
}

bool Connection::handle_write() {
    if (closed_) {
        return false;
    }
    return write_out();
}

bool Connection::write_out() {
    while (out_.readable() > 0) {
        // MSG_NOSIGNAL：对端已关闭时返回 EPIPE 而不是触发 SIGPIPE。
        // 没有它的话，一次"往已关闭的 socket 写"就能杀掉整个服务进程。
        const ssize_t n = ::send(fd_.get(), out_.data(), out_.readable(), MSG_NOSIGNAL);

        if (n > 0) {
            out_.retrieve(static_cast<std::size_t>(n));
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;  // 内核发送缓冲满了，先留着，等 EPOLLOUT
        }

        closed_ = true;
        return false;
    }
    return true;
}

void Connection::send(std::string_view data) {
    if (closed_) {
        return;
    }

    // 快路径：写缓冲为空时直接写，避免一次多余的拷贝和一次 EPOLLOUT 往返。
    if (out_.readable() == 0) {
        const ssize_t n = ::send(fd_.get(), data.data(), data.size(), MSG_NOSIGNAL);
        if (n > 0) {
            data.remove_prefix(static_cast<std::size_t>(n));
            if (data.empty()) {
                return;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            closed_ = true;
            return;
        }
    }

    // 慢路径：写不完的部分进缓冲，由 EPOLLOUT 事件继续推。
    out_.append(data);
}

void Connection::shutdown() noexcept {
    closed_ = true;
}

void Connection::shutdown_write() noexcept {
    if (fd_.valid()) {
        ::shutdown(fd_.get(), SHUT_WR);
    }
}

}  // namespace chatroom::net
