#pragma once

#include <cstdint>
#include <vector>

#include <sys/epoll.h>

#include "chatroom/platform/unique_fd.h"

namespace chatroom::net {

// epoll 的薄封装。
//
// 存在的意义是把"系统调用"集中在一个地方：其余代码只看到 add/mod/del/wait，
// 不直接碰 epoll 的细节。这样以后换 IO 模型（例如 M6 加 eventfd 唤醒）
// 只需要改这一个文件。
class Poller {
public:
    Poller();
    ~Poller();

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    void add(int fd, std::uint32_t events);
    void mod(int fd, std::uint32_t events);
    void del(int fd);

    // 等待就绪事件。返回本次就绪的个数（0 表示超时，-1 表示出错）。
    int wait(int timeout_ms);

    const epoll_event* events() const noexcept { return events_.data(); }

private:
    platform::UniqueFd epfd_;
    // 一次最多返回多少个事件。满了只会让剩余事件留到下一轮，不会丢。
    static constexpr int kMaxEvents = 256;
    std::vector<epoll_event> events_{kMaxEvents};
};

}  // namespace chatroom::net
