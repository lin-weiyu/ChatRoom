// acceptor.h — 接受新连接
//
// 所属层：net
// 被谁使用：Server
//
// 只做一件事：把监听 fd 上的待处理连接全部取走。
// 它不持有 Poller，也不决定新连接怎么活——那是 Server 的职责。
//
// 这里同时承担一个**契约责任**：交出去的每个客户端 fd 都已经设成
// 非阻塞 + TCP_NODELAY。Connection 依赖这个前提写代码。

#pragma once

#include "chatroom/platform/unique_fd.h"

#include <cstdint>
#include <functional>
#include <string>

namespace chatroom::net {

/// 监听并接受新连接。
class Acceptor {
public:
    /// 一次 accept 的结果。
    struct Accepted {
        platform::UniqueFd fd;  ///< 已设为非阻塞的客户端 fd
        std::string peer;       ///< "ip:port"，仅用于日志
    };

    using AcceptHandler = std::function<void(Accepted&&)>;

    /// 绑定并开始监听。失败时 valid() 返回 false。
    explicit Acceptor(std::uint16_t port, int backlog = 128);

    bool valid() const noexcept {
        return listen_fd_.valid();
    }

    /// 监听 fd，用于注册到 Poller。
    int fd() const noexcept {
        return listen_fd_.get();
    }

    /// 实际监听端口（构造时传 0 才有意义的查询）。
    std::uint16_t port() const noexcept;

    /// 循环 accept 直到返回 EAGAIN。
    ///
    /// 为什么必须循环：Level-Triggered 模式下只 accept 一次也能跑，但连接
    /// 暴增时会造成"每次事件只取一个"的延迟累积；Edge-Triggered 模式下
    /// 不循环则直接丢连接。
    ///
    /// 遇到 EMFILE（进程 fd 用尽）时直接返回，不退出进程——此时能做的
    /// 只有让新连接在队列里等着，同时把日志打出来。
    void accept_all(const AcceptHandler& handler);

private:
    platform::UniqueFd listen_fd_;
};

}  // namespace chatroom::net
