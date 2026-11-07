#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "chatroom/platform/unique_fd.h"

namespace chatroom::net {

// 监听并接受新连接。
//
// 只做一件事：把监听 fd 上的待处理连接全部取走。不持有 Poller，
// 也不决定新连接怎么活——那是 Server 的职责。
class Acceptor {
public:
    struct Accepted {
        platform::UniqueFd fd;
        std::string peer;
    };

    using AcceptHandler = std::function<void(Accepted&&)>;

    explicit Acceptor(std::uint16_t port, int backlog = 128);

    bool valid() const noexcept { return listen_fd_.valid(); }

    int fd() const noexcept { return listen_fd_.get(); }

    // 内核分配的真实端口（构造时传 0 才有意义）
    std::uint16_t port() const noexcept;

    // 循环 accept 直到 EAGAIN。
    //
    // 必须循环：水平触发下只 accept 一次也能跑，但连接猛增时会造成
    // "每次都只取一个"的延迟累积；边缘触发下不循环则是直接丢连接。
    void accept_all(const AcceptHandler& handler);

private:
    platform::UniqueFd listen_fd_;
};

}  // namespace chatroom::net
