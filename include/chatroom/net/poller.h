// poller.h — epoll 的薄封装
//
// 所属层：net
// 被谁使用：Server（唯一持有 Poller 的地方）
//
// 存在的意义：把系统调用集中在一处。其余代码只看到 add/mod/del/wait，
// 不直接碰 epoll 的细节。以后要换 IO 模型（比如加 eventfd 唤醒、换 io_uring），
// 只需要改这一个文件。
//
// epoll 工作模型（三个系统调用）：
//   epoll_create1  创建一张"关注列表"
//   epoll_ctl      往列表里增删改关注的 fd 和事件类型
//   epoll_wait     阻塞等待，返回"就绪的那几个"

#pragma once

#include "chatroom/platform/unique_fd.h"

#include <cstdint>
#include <sys/epoll.h>
#include <vector>

namespace chatroom::net {

/// epoll 实例的封装。非线程安全，只在 IO 线程里使用。
class Poller {
public:
    Poller();
    ~Poller();

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    /// 注册关心的 fd 和事件（EPOLLIN / EPOLLOUT / EPOLLRDHUP）。
    /// 失败时静默返回——调用方保证不会重复注册同一个 fd。
    void add(int fd, std::uint32_t events);

    /// 修改某 fd 的关心事件。典型用法：有积压数据时加上 EPOLLOUT。
    void mod(int fd, std::uint32_t events);

    /// 从关注列表移除。对已经 close 的 fd 调用会返回 EBADF，属预期行为。
    void del(int fd);

    /// 等待就绪事件。
    ///
    /// @param timeout_ms  -1 永久阻塞；0 立即返回；>0 最多等这么多毫秒
    /// @return           就绪事件个数；0 表示超时；被信号打断时也返回 0
    int wait(int timeout_ms);

    /// 返回本次 wait() 就绪的事件数组，配合 wait() 的返回值使用。
    const epoll_event* events() const noexcept {
        return events_.data();
    }

private:
    platform::UniqueFd epfd_;

    /// 一次最多取回多少个事件。取满只会让剩余事件留到下一轮，不会丢。
    static constexpr int kMaxEvents = 256;
    std::vector<epoll_event> events_{kMaxEvents};
};

}  // namespace chatroom::net
