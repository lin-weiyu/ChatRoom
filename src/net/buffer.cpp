#include "chatroom/net/buffer.h"

#include <algorithm>
#include <cstring>

namespace chatroom::net {

void Buffer::ensure_writable(std::size_t len) {
    if (buf_.size() - write_index_ >= len) {
        return;
    }

    const std::size_t readable_len = readable();

    // 头部有空洞、且合并后装得下：压缩，不扩容
    if (read_index_ > 0 && readable_len + len <= buf_.size()) {
        compact();
        return;
    }

    // 真的不够：按 2 倍增长，避免每次 append 都扩容
    std::size_t needed = readable_len + len;
    std::size_t new_cap = buf_.size() * 2;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    if (read_index_ > 0) {
        compact();
    }

    buf_.resize(new_cap);
}

void Buffer::compact() noexcept {
    const std::size_t readable_len = readable();
    if (read_index_ > 0) {
        std::memmove(buf_.data(), buf_.data() + read_index_, readable_len);
        read_index_ = 0;
        write_index_ = readable_len;
    }
}

void Buffer::append(const char* data, std::size_t len) {
    ensure_writable(len);
    std::memcpy(buf_.data() + write_index_, data, len);
    write_index_ += len;
}

void Buffer::retrieve(std::size_t n) noexcept {
    n = std::min(n, readable());
    read_index_ += n;
    if (read_index_ == write_index_) {
        read_index_ = 0;
        write_index_ = 0;
    }
}

}  // namespace chatroom::net
