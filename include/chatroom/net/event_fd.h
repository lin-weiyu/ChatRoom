// event_fd.h — 跨线程唤醒 epoll 的计数器
//
// 所属层：net（Linux I/O 原语，和 epoll 同源）
// 被谁使用：app::ChatServer（工作线程 → IO 线程的结果通知）
//
// 问题：IO 线程阻塞在 epoll_wait 里，工作线程算完了结果放进队列，
//       但没有人告诉 IO 线程"有新结果"，它就只能靠超时轮询。
//
// 解法：eventfd 是内核提供的一个 8 字节计数器，可读可写，能注册进 epoll。
//       写一次（+1）它就变成可读，epoll_wait 立刻返回。
//
// 为什么不用管道：管道要两个 fd（读写各一），还要处理 EOF；
//                 eventfd 一个 fd、计数语义天然不丢事件。

#pragma once

#include "chatroom/platform/unique_fd.h"

#include <cstdint>

namespace chatroom::net {

/// 基于 Linux eventfd(2) 的跨线程信号。非阻塞 + CLOEXEC。
class EventFd {
public:
    /// 创建 eventfd。失败时 valid() 返回 false。
    EventFd();
    ~EventFd();

    EventFd(const EventFd&) = delete;
    EventFd& operator=(const EventFd&) = delete;

    bool valid() const noexcept {
        return fd_.valid();
    }

    /// 用于注册到 Poller。
    int fd() const noexcept {
        return fd_.get();
    }

    /// 任意线程可调用：计数器 +1，唤醒等待方。
    /// 写失败（计数器饱和 / fd 已关闭）不影响"有人会去检查队列"的语义。
    void notify() noexcept;

    /// IO 线程调用：把累计计数一次读走，清掉可读状态。
    void drain() noexcept;

private:
    platform::UniqueFd fd_;
};

}  // namespace chatroom::net
