#include "chatroom/net/acceptor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <sstream>

#include "chatroom/net/socket.h"

namespace chatroom::net {

Acceptor::Acceptor(std::uint16_t port, int backlog)
    : listen_fd_(create_listener(port, backlog)) {}

std::uint16_t Acceptor::port() const noexcept { return local_port(listen_fd_.get()); }

void Acceptor::accept_all(const AcceptHandler& handler) {
    for (;;) {
        sockaddr_in peer_addr{};
        socklen_t peer_len = sizeof(peer_addr);

        const int fd = ::accept(listen_fd_.get(), reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);

        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;  // 没有更多连接了
            }
            if (errno == EINTR) {
                continue;
            }
            // EMFILE/ENFILE：fd 用完了。此时不能直接退出循环（会把 CPU 烧死），
            // 也不能什么都不做。真实项目里会保留一个"备用 fd"，腾出来 accept
            // 后立刻 close，给客户端一个干净的 RST。
            return;
        }

        set_nonblocking(fd);
        set_tcp_nodelay(fd);

        std::ostringstream peer;
        peer << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port);

        handler(Accepted{platform::UniqueFd{fd}, peer.str()});
    }
}

}  // namespace chatroom::net
