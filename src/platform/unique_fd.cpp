#include "chatroom/platform/unique_fd.h"

#include <utility>

#include <unistd.h>

namespace chatroom::platform {

UniqueFd::~UniqueFd() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
        // 赋值前必须先释放自己持有的 fd，否则它也泄漏了。
        reset(other.release());
    }
    return *this;
}

int UniqueFd::release() noexcept {
    const int fd = fd_;
    fd_ = -1;
    return fd;
}

void UniqueFd::reset(int fd) noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
    }
    fd_ = fd;
}

}  // namespace chatroom::platform
