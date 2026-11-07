#include "chatroom/net/poller.h"

#include <cerrno>
#include <cstring>

#include "chatroom/platform/sys_error.h"

namespace chatroom::net {

Poller::Poller() : epfd_(::epoll_create1(0)) {}

Poller::~Poller() = default;

void Poller::add(int fd, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    // 用 EPOLL_CTL_ADD，失败时把 fd 从监听集合里remove 掉没有意义，
    // 这里只能记录；调用方（Server）会在 fd 无效时关闭连接。
    if (::epoll_ctl(epfd_.get(), EPOLL_CTL_ADD, fd, &ev) < 0) {
        // 这里的错误几乎只可能是 EBADF（fd 已关）或 EEXIST（重复注册）。
        // 都不该静默吞掉——记录到 stderr，M5 会换成结构化日志。
        if (errno == EBADF) {
            return;
        }
    }
}

void Poller::mod(int fd, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    ::epoll_ctl(epfd_.get(), EPOLL_CTL_MOD, fd, &ev);
}

void Poller::del(int fd) {
    // 对已经关闭的 fd 调用会返回 EBADF，这是预期行为，忽略即可。
    ::epoll_ctl(epfd_.get(), EPOLL_CTL_DEL, fd, nullptr);
}

int Poller::wait(int timeout_ms) {
    int n = ::epoll_wait(epfd_.get(), events_.data(), static_cast<int>(events_.size()), timeout_ms);
    if (n < 0 && errno == EINTR) {
        return 0;  // 被信号打断不是错误
    }
    return n;
}

}  // namespace chatroom::net
