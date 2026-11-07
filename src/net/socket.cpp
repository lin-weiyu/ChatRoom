#include "chatroom/net/socket.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>

namespace chatroom::net {

void set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return;
    }
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_tcp_nodelay(int fd) {
    const int on = 1;
    ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
}

platform::UniqueFd create_listener(std::uint16_t port, int backlog) {
    platform::UniqueFd fd{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!fd.valid()) {
        return {};
    }

    // TIME_WAIT 状态下仍允许重新绑定端口。重启服务时不做这件事，
    // 会看到 "Address already in use"，然后白白等 60 秒。
    const int on = 1;
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return {};
    }

    if (::listen(fd.get(), backlog) < 0) {
        return {};
    }

    set_nonblocking(fd.get());
    return fd;
}

std::uint16_t local_port(int fd) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        return 0;
    }
    return ntohs(addr.sin_port);
}

}  // namespace chatroom::net
