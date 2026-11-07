#pragma once

#include "chatroom/protocol/frame_codec.h"

#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

#include <poll.h>
#include <unistd.h>

namespace chatroom::testing {

// 同步阻塞的测试客户端。
//
// 故意用最简单的方式实现：connect / send / recv-with-timeout。
// 集成测试要验证的是服务端行为，客户端越笨越好——聪明的客户端会掩盖
// 服务端的问题。
class TestClient {
public:
    TestClient() = default;
    ~TestClient() {
        close();
    }

    TestClient(const TestClient&) = delete;
    TestClient& operator=(const TestClient&) = delete;

    bool connect_to(std::uint16_t port) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }
        return true;
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool send_frame(const protocol::Frame& frame) {
        const std::string bytes = protocol::encode(frame);
        if (bytes.empty()) {
            return false;
        }
        std::size_t sent = 0;
        while (sent < bytes.size()) {
            const ssize_t n = ::send(fd_, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
            if (n <= 0) {
                return false;
            }
            sent += static_cast<std::size_t>(n);
        }
        return true;
    }

    // 直接写原始字节，用于构造非法的协议帧
    bool send_raw(const void* data, std::size_t len) {
        std::size_t sent = 0;
        const char* p = static_cast<const char*>(data);
        while (sent < len) {
            const ssize_t n = ::send(fd_, p + sent, len - sent, MSG_NOSIGNAL);
            if (n <= 0) {
                return false;
            }
            sent += static_cast<std::size_t>(n);
        }
        return true;
    }

    // 收一帧，超时返回 false。timeout_ms 是"总共最多等多久"，
    // 不是每次 recv 的超时。
    bool recv_frame(protocol::Frame& frame, int timeout_ms = 2000) {
        for (;;) {
            const auto r = protocol::try_decode(rx_);
            if (r.status == protocol::DecodeStatus::kOk) {
                rx_.erase(0, r.consumed);
                frame = r.frame;
                return true;
            }
            if (r.status == protocol::DecodeStatus::kError) {
                return false;
            }

            pollfd pfd{};
            pfd.fd = fd_;
            pfd.events = POLLIN;

            const int ready = ::poll(&pfd, 1, timeout_ms);
            if (ready <= 0) {
                return false;
            }

            char buf[4096];
            const ssize_t got = ::recv(fd_, buf, sizeof(buf), 0);
            if (got <= 0) {
                return false;
            }
            rx_.append(buf, static_cast<std::size_t>(got));
        }
    }

    // 一直读到连接被对端关闭为止，返回是否观察到 EOF
    bool wait_for_eof(int timeout_ms = 2000) {
        for (;;) {
            pollfd pfd{};
            pfd.fd = fd_;
            pfd.events = POLLIN;

            const int ready = ::poll(&pfd, 1, timeout_ms);
            if (ready <= 0) {
                return false;
            }
            char buf[4096];
            const ssize_t got = ::recv(fd_, buf, sizeof(buf), 0);
            if (got == 0) {
                return true;
            }
            if (got < 0) {
                return false;
            }
        }
    }

private:
    int fd_ = -1;
    std::string rx_;
};

}  // namespace chatroom::testing
