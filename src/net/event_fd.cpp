#include "chatroom/net/event_fd.h"

#include <cstdint>
#include <sys/eventfd.h>

#include <unistd.h>

namespace chatroom::net {

EventFd::EventFd() : fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {}

EventFd::~EventFd() = default;

void EventFd::notify() noexcept {
    if (!fd_.valid()) {
        return;
    }
    const std::uint64_t one = 1;
    // 写失败只可能是 EAGAIN（计数器饱和）或 fd 已关闭，两种情况都不影响
    // "有人会去检查队列"这个语义，忽略即可。
    const ssize_t ignored = ::write(fd_.get(), &one, sizeof(one));
    (void)ignored;
}

void EventFd::drain() noexcept {
    if (!fd_.valid()) {
        return;
    }
    std::uint64_t value = 0;
    const ssize_t ignored = ::read(fd_.get(), &value, sizeof(value));
    (void)ignored;
}

}  // namespace chatroom::net
