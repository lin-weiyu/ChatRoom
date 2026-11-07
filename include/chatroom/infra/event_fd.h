#pragma once

#include <cstdint>

#include "chatroom/platform/unique_fd.h"

namespace chatroom::infra {

// eventfd：内核提供的一个 8 字节计数器，可读可写，能直接注册进 epoll。
//
// 用途是"跨线程叫醒 epoll"。
//
// 为什么不能直接 epoll_ctl 一个普通 fd：工作线程要通知 IO 线程"有结果了"，
// 但 epoll_wait 正在阻塞，没有任何 fd 就绪它就不会返回。eventfd 提供了一个
// 合法的、可读的 fd，写一次就让它变成可读，epoll 立刻返回。
//
// 为什么不 fork 一个管道：管道要两个 fd，读写方向各一；eventfd 一个 fd
// 就够了，且计数语义不会丢事件（写 N 次，读一次就能把累计值读走）。
class EventFd {
public:
    EventFd();
    ~EventFd();

    EventFd(const EventFd&) = delete;
    EventFd& operator=(const EventFd&) = delete;

    bool valid() const noexcept { return fd_.valid(); }

    int fd() const noexcept { return fd_.get(); }

    // 任意线程可调用：把计数器 +1，唤醒等待方
    void notify() noexcept;

    // IO 线程调用：把累计值读走，清掉可读状态
    void drain() noexcept;

private:
    platform::UniqueFd fd_;
};

}  // namespace chatroom::infra
