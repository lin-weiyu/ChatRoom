// connection.cpp — 连接的读写实现
//
// 读这段代码前需要知道的三个系统调用事实（详见 docs/basics.md）：
//   1. fd 是内核文件表的下标，你手上只有数字
//   2. recv 把内核接收缓冲区里的数据搬到你的 buf，一次搬多少不由你决定
//   3. 非阻塞 fd 在读空时返回 -1 且 errno=EAGAIN，而不是睡住
//
// 这份代码里的所有循环，都是为了消化"一次 recv 拿到的不是一个完整消息"
// 这个事实。

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

// ---------------------------------------------------------------------------
// 读：把内核缓冲区的数据尽可能全部搬进来
// ---------------------------------------------------------------------------
bool Connection::handle_read() {
    if (closed_) {
        return false;
    }

    // 一次最多搬 64KB。这个大小是吞吐和栈占用的折中：
    // 太小要循环很多次，太大会吃掉线程栈。
    char buf[65536];

    // 为什么是 for(;;) 而不是 if：
    //   epoll 通知"可读"只意味着"至少有一个字节"，不保证"数据读完了"。
    //   一次 recv 之后缓冲区里可能还有，所以必须继续读，直到内核说
    //   "没了"（EAGAIN）。只读一次会让吞吐减半，也把半包处理搞复杂。
    for (;;) {
        // ssize_t 是"有符号的 size_t"——因为返回值要能表示 -1。
        const ssize_t n = ::recv(fd_.get(), buf, sizeof(buf), 0);

        if (n > 0) {
            // 拿到了 n 字节（可能远小于 65536）。先存进读缓冲，再交给上层解析。
            // 上层可能只消费其中一部分（半个帧），剩下的留在缓冲里等下次补齐。
            in_.append(buf, static_cast<std::size_t>(n));
            if (on_message_) {
                on_message_(*this, in_.view());
            }
            continue;  // 可能还有，继续读
        }

        if (n == 0) {
            // 返回 0 表示对端发了 FIN（有序关闭）。这是正常结束，不是错误：
            // 对方把话说完、挂了电话，不是线路故障。
            closed_ = true;
            return false;  // 出口 ①
        }

        // 走到这里 n == -1，此时 errno 才有意义。
        if (errno == EINTR) {
            // 被信号打断（比如 Ctrl-C 的信号处理器）。系统调用根本没执行，
            // 不是错误，重试即可。
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 读空了。这就是非阻塞的价值：内核说"没货了"，而不是让我们睡住。
            // 注意这是正常状态——连接完全健康。
            return true;  // 出口 ②
        }

        // 剩下的都是真错误：ECONNRESET（对方强断）、EBADF、EIO 等。
        closed_ = true;
        return false;  // 出口 ③
    }
}

// ---------------------------------------------------------------------------
// 写：把写缓冲里的数据尽量推给内核
// ---------------------------------------------------------------------------
bool Connection::handle_write() {
    if (closed_) {
        return false;
    }
    return write_out();
}

bool Connection::write_out() {
    while (out_.readable() > 0) {
        // MSG_NOSIGNAL：对端已关闭时返回 EPIPE，而不是触发 SIGPIPE。
        // 没有这个标志的话，一次"往已关闭的 socket 写"就能杀掉整个进程——
        // SIGPIPE 的默认动作是终止进程。
        const ssize_t n = ::send(fd_.get(), out_.data(), out_.readable(), MSG_NOSIGNAL);

        if (n > 0) {
            // send 的返回值是"写进去多少字节"，不是"成功/失败"。
            // 内核发送缓冲区装不下时只收一部分，剩下的必须留着下次再发。
            out_.retrieve(static_cast<std::size_t>(n));
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // 发送缓冲区满了。数据留在 out_ 里，等 EPOLLOUT 事件再来推。
            // 这就是"背压"：慢客户端不会拖垮别人，只是自己积压。
            return true;
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

    // 快路径：写缓冲为空时先直接写。一次写完就完全不需要进队列，
    // 也不需要注册 EPOLLOUT——省掉一次事件循环往返。
    // 聊天场景下小消息占绝大多数，所以走的大多是这条路。
    if (out_.readable() == 0) {
        const ssize_t n = ::send(fd_.get(), data.data(), data.size(), MSG_NOSIGNAL);
        if (n > 0) {
            data.remove_prefix(static_cast<std::size_t>(n));
            if (data.empty()) {
                return;  // 全写进去了，收工
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            // 不是"暂时写不进去"，而是真的坏了。
            closed_ = true;
            return;
        }
    }

    // 慢路径：没写完的部分进缓冲。Server 会看到 want_write() 为真，
    // 注册 EPOLLOUT，等内核能收了再继续推。
    out_.append(data);
}

void Connection::shutdown() noexcept {
    closed_ = true;
}

void Connection::shutdown_write() noexcept {
    if (fd_.valid()) {
        // 只关写方向：发 FIN 告诉对方"我没有更多数据了"，
        // 但读方向还开着，仍能收对方的收尾消息。
        ::shutdown(fd_.get(), SHUT_WR);
    }
}

}  // namespace chatroom::net
