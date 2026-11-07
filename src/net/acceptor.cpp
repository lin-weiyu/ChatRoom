// acceptor.cpp — accept 循环
//
// 这里有一个容易被忽略的责任：交出去的 fd 必须设成非阻塞。
// Connection 的全部读循环都建立在"没有数据时返回 EAGAIN"这个前提上，
// 如果这里漏了，服务器会在第一次"数据不够读"时整体卡死。

#include "chatroom/net/acceptor.h"

#include "chatroom/net/socket.h"

#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>

#include <unistd.h>

namespace chatroom::net {

Acceptor::Acceptor(std::uint16_t port, int backlog) : listen_fd_(create_listener(port, backlog)) {}

std::uint16_t Acceptor::port() const noexcept {
    return local_port(listen_fd_.get());
}

void Acceptor::accept_all(const AcceptHandler& handler) {
    for (;;) {
        sockaddr_in peer_addr{};
        socklen_t peer_len = sizeof(peer_addr);

        // accept 从全连接队列里取出一个已完成三次握手的连接。
        // 它返回一个新的 fd，和监听 fd 是两个不同的东西：
        //   监听 fd  = 门（只负责迎客）
        //   客户端 fd = 每个客人专属的连接
        const int fd =
            ::accept(listen_fd_.get(), reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);

        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;  // 队列空了，正常退出循环
            }
            if (errno == EINTR) {
                continue;
            }
            // EMFILE/ENFILE：进程 fd 用完了。
            // 此时不能继续循环（会空转烧 CPU），也不能直接退出进程。
            // 工业级做法是提前预留一个"备用 fd"：腾出来 accept 后立刻 close，
            // 给客户端一个干净的 RST，而不是让它一直挂着。
            return;
        }

        // 关键契约：交给 Connection 之前，fd 必须已经非阻塞。
        set_nonblocking(fd);
        // 聊天消息很小、对延迟敏感，关掉 Nagle 避免攒包。
        set_tcp_nodelay(fd);

        std::ostringstream peer;
        peer << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port);

        handler(Accepted{platform::UniqueFd{fd}, peer.str()});
    }
}

}  // namespace chatroom::net
