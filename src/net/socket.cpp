// socket.cpp — 创建 socket、设置属性
//
// 这里集中了所有"创建 fd 并配置它"的系统调用，业务代码不用再碰它们。

#include "chatroom/net/socket.h"

#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <unistd.h>

namespace chatroom::net {

void set_nonblocking(int fd) {
    // 标准姿势：先 F_GETFL 读出当前标志，再加上 O_NONBLOCK 写回去。
    //
    // 为什么不能直接 fcntl(fd, F_SETFL, O_NONBLOCK)：
    //   F_SETFL 是整体替换，直接写会把已有的其他标志（比如 O_APPEND）
    //   一起清掉。先读后写是改标志位的通用做法。
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return;
    }
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_tcp_nodelay(int fd) {
    const int on = 1;
    // TCP_NODELAY 关闭 Nagle 算法。Nagle 会把小包攒起来一起发，
    // 省带宽但增加延迟——对聊天场景是纯负收益。
    ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
}

platform::UniqueFd create_listener(std::uint16_t port, int backlog) {
    // socket() 返回的只是一个数字（fd），真正的 socket 对象在内核里。
    platform::UniqueFd fd{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!fd.valid()) {
        return {};
    }

    // SO_REUSEADDR：服务重启时，旧连接还处于 TIME_WAIT 状态，地址仍被占用。
    // 没有这个选项会看到 "Address already in use"，然后白白等 60 秒。
    const int on = 1;
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡
    // 端口必须以网络字节序（大端）填入。htons = host to network，short。
    // 小端机器上如果不转换，12345 会变成 14640 之类的怪值。
    addr.sin_port = htons(port);

    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return {};
    }

    // listen 不是"配置"，是"开门营业"：让 socket 从主动连接方变成被动接受方，
    // 内核开始把完成的握手放进全连接队列。
    if (::listen(fd.get(), backlog) < 0) {
        return {};
    }

    // 监听 fd 也要非阻塞：这样 accept 在队列为空时返回 EAGAIN，
    // 而不是把整个事件循环睡死。
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
