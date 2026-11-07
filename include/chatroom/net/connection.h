// connection.h — 一个 TCP 连接的全部状态
//
// 所属层：net
// 被谁使用：Server（创建与销毁）、ChatServer（收发数据）
//
// 拥有的东西：fd、读缓冲、写缓冲、对端地址、关闭标志。
// 不知道的东西：聊天、房间、用户——那些在 chat 层。
//
// 三个最容易写错的地方，本类都处理了：
//   1. recv/send 可能被信号打断（EINTR），必须重试而不是当错误
//   2. send 的返回值是"写进去多少"，不是"成功/失败"
//   3. 对端关闭后往 socket 写会触发 SIGPIPE，默认行为是杀掉整个进程

#pragma once

#include "chatroom/net/buffer.h"
#include "chatroom/platform/unique_fd.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace chatroom::net {

/// 一条已建立的 TCP 连接。非线程安全：只在 IO 线程里访问。
///
/// 前置条件：构造时传入的 fd 必须已经是非阻塞的（由 Acceptor 保证）。
class Connection {
public:
    /// 收到数据时调用。
    ///
    /// 参数 view 指向读缓冲里的**全部**可读字节，可能是半个消息，
    /// 也可能是好几个消息粘在一起。回调必须通过 input().retrieve(n)
    /// 告诉 Connection"我消费了多少"，否则数据会被重复处理。
    using MessageHandler = std::function<void(Connection&, std::string_view)>;

    Connection(platform::UniqueFd fd, std::string peer);

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    int fd() const noexcept {
        return fd_.get();
    }

    /// 对端地址，形如 "127.0.0.1:54321"，只用于日志。
    const std::string& peer() const noexcept {
        return peer_;
    }

    void set_message_handler(MessageHandler on_message);

    /// 处理可读事件：循环 recv 直到 EAGAIN。
    ///
    /// @return false 表示连接已经不可用（对端关闭或读出错），
    ///         调用方应当把它从 epoll 摘掉并销毁。
    ///
    /// 这个函数是整个网络层最核心的一段代码。它依赖三个事实：
    ///   - fd 非阻塞：没有数据时 recv 返回 -1/errno=EAGAIN，而不是睡住
    ///   - recv 返回 0 表示对端发了 FIN（正常关闭，不是错误）
    ///   - 一次 recv 拿到的字节数不确定：可能半个消息，也可能好几个
    bool handle_read();

    /// 处理可写事件：把写缓冲里的数据尽量发出去。
    /// @return false 表示写失败，连接应当关闭。
    bool handle_write();

    /// 把数据放进发送队列。
    ///
    /// 快路径：写缓冲为空时先直接 send，能一次写完就完全不进队列。
    /// 慢路径：send 只写进去一部分（或返回 EAGAIN），剩下的追加到写缓冲，
    ///         由 Server 注册 EPOLLOUT 继续推。
    void send(std::string_view data);

    /// 写缓冲里是否还有积压。有积压才需要注册 EPOLLOUT——
    /// 常驻注册会让 epoll 在每次可写时都唤醒我们，白白烧 CPU。
    bool want_write() const noexcept {
        return out_.readable() > 0;
    }

    bool closed() const noexcept {
        return closed_;
    }

    /// 读缓冲。上层从这里取数据并解析协议帧。
    Buffer& input() noexcept {
        return in_;
    }

    /// 立即标记为关闭。Server 会把它从 epoll 移除并销毁。
    void shutdown() noexcept;

    /// 只关闭写方向：发一个 FIN，但仍继续读取对端数据。
    ///
    /// 这是"优雅关闭"的正确做法。直接 close() 的问题是：如果接收缓冲里
    /// 还有未读数据，内核会发 RST 而不是 FIN，对端刚收到的数据可能被丢弃。
    void shutdown_write() noexcept;

private:
    /// 把写缓冲里的数据尽量发出去，返回是否还活着。
    bool write_out();

    platform::UniqueFd fd_;
    std::string peer_;

    Buffer in_;   ///< 从 socket 读进来的字节，尚未被协议层消费
    Buffer out_;  ///< 待发送的字节，尚未被内核收下

    bool closed_ = false;

    MessageHandler on_message_;
};

}  // namespace chatroom::net
