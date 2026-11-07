#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "chatroom/net/buffer.h"
#include "chatroom/platform/unique_fd.h"

namespace chatroom::net {

// 一个 TCP 连接。
//
// 拥有的东西：fd（UniqueFd）、读缓冲、写缓冲、对端地址。
// 不知道的东西：聊天、房间、用户——那些在 app 层。
//
// 三个容易写错的地方，这里都处理了：
//   1. recv/send 可能被信号打断（EINTR），必须重试而不是当成错误
//   2. send 的返回值是"写进去多少"，不是"成功/失败"
//   3. 对端关闭后往 socket 写会触发 SIGPIPE，默认行为是杀掉进程
class Connection {
public:
    // 收到数据时调用。view 指向读缓冲中的全部可读字节，
    // 回调里通过 retrieve() 决定消费多少（M1 的协议解析就靠这个）。
    using MessageHandler = std::function<void(Connection&, std::string_view)>;

    Connection(platform::UniqueFd fd, std::string peer);

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    int fd() const noexcept { return fd_.get(); }

    const std::string& peer() const noexcept { return peer_; }

    void set_message_handler(MessageHandler on_message);

    // 处理可读事件。返回 false 表示连接已经不可用。
    bool handle_read();

    // 处理可写事件，把写缓冲里的数据尽量发出去。
    bool handle_write();

    // 把数据放进发送队列；能直接写成功就不进队列。
    void send(std::string_view data);

    // 写缓冲里是否还有积压——有积压才需要注册 EPOLLOUT
    bool want_write() const noexcept { return out_.readable() > 0; }

    bool closed() const noexcept { return closed_; }

    // 访问读缓冲，供上层解析协议
    Buffer& input() noexcept { return in_; }

    // 主动关闭（立即停止收发，由 Server 移除）
    void shutdown() noexcept;

    // 只关写方向：发一个 FIN，但仍继续读取对端的数据。
    //
    // 这是"优雅关闭"的正确做法。直接 close() 的问题是：如果接收缓冲里
    // 还有未读数据，内核会发 RST 而不是 FIN，对端刚收到的数据可能被丢弃。
    void shutdown_write() noexcept;

private:
    bool write_out();

    platform::UniqueFd fd_;
    std::string peer_;

    Buffer in_;
    Buffer out_;

    bool closed_ = false;

    MessageHandler on_message_;
};

}  // namespace chatroom::net
