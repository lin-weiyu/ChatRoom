// 命令行聊天客户端。
//
// 存在的理由有两个：
//   1. 长度前缀协议没法用 nc 手打，需要工具才能调试
//   2. 集成测试和压测都需要一个"懂协议"的客户端
//
// 用法：
//   chat_cli [host] [port]            交互模式
//   printf '/join alice\n/msg hi\n/quit\n' | chat_cli   ← 脚本模式
//
// 命令：
//   /join <nick>    /nick <nick>     /msg <text>
//   /dm <user> <text>                /list
//   /joinroom <room>  /leaveroom <room>
//   /quit

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "chatroom/protocol/frame_codec.h"

namespace {

using chatroom::protocol::Frame;
using chatroom::protocol::MessageType;

struct Client {
    int fd = -1;
    std::string rx;  // 接收缓冲：TCP 是字节流，必须自己攒帧

    bool connect_to(const std::string& host, std::uint16_t port) {
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
            return false;
        }
        return ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
    }

    void send_frame(const Frame& frame) {
        const std::string bytes = chatroom::protocol::encode(frame);
        if (!bytes.empty()) {
            ::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL);
        }
    }

    // 把接收缓冲里所有完整的帧都取出来打印
    void drain() {
        for (;;) {
            const auto r = chatroom::protocol::try_decode(rx);
            if (r.status == chatroom::protocol::DecodeStatus::kIncomplete) {
                return;
            }
            if (r.status == chatroom::protocol::DecodeStatus::kError) {
                std::cout << "!! protocol error: " << r.error << "\n";
                rx.clear();
                return;
            }
            rx.erase(0, r.consumed);

            std::cout << "[" << static_cast<int>(r.frame.type) << "]";
            for (const auto& field : r.frame.fields) {
                std::cout << " " << field;
            }
            std::cout << "\n";
            std::cout.flush();
        }
    }
};

MessageType type_of(const std::string& level) {
    if (level == "/join") return MessageType::kJoin;
    if (level == "/nick") return MessageType::kNick;
    if (level == "/msg") return MessageType::kMsg;
    if (level == "/dm") return MessageType::kDm;
    if (level == "/quit") return MessageType::kQuit;
    if (level == "/joinroom") return MessageType::kJoinRoom;
    if (level == "/leaveroom") return MessageType::kLeaveRoom;
    if (level == "/list") return MessageType::kList;
    return MessageType::kMsg;
}

// 解析一行命令，返回是否应该退出
bool handle_line(Client& client, const std::string& line) {
    if (line.empty()) {
        return false;
    }

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    Frame frame;
    frame.type = type_of(cmd);

    if (cmd == "/quit") {
        client.send_frame(frame);
        return true;
    }
    if (cmd == "/list") {
        client.send_frame(frame);
        return false;
    }
    if (cmd == "/join" || cmd == "/nick" || cmd == "/joinroom" || cmd == "/leaveroom") {
        std::string arg;
        iss >> arg;
        frame.fields = {arg};
        client.send_frame(frame);
        return false;
    }
    if (cmd == "/dm") {
        std::string target;
        iss >> target;
        std::string rest;
        std::getline(iss, rest);
        if (!rest.empty() && rest.front() == ' ') {
            rest.erase(0, 1);
        }
        frame.fields = {target, rest};
        client.send_frame(frame);
        return false;
    }
    if (cmd == "/msg") {
        std::string rest;
        std::getline(iss, rest);
        if (!rest.empty() && rest.front() == ' ') {
            rest.erase(0, 1);
        }
        frame.fields = {"", rest};
        client.send_frame(frame);
        return false;
    }

    std::cout << "unknown command: " << cmd << "\n";
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::uint16_t port = 12345;
    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<std::uint16_t>(std::atoi(argv[2]));

    Client client;
    if (!client.connect_to(host, port)) {
        std::cerr << "connect failed\n";
        return 1;
    }

    std::cout << "connected to " << host << ":" << port << "\n";
    std::cout.flush();

    std::string pending;
    bool stdin_open = true;

    for (;;) {
        pollfd fds[2];
        fds[0].fd = client.fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        fds[1].fd = stdin_open ? 0 : -1;
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        const int n = ::poll(fds, 2, 500);
        if (n < 0) {
            break;
        }

        if ((fds[0].revents & (POLLERR | POLLHUP)) != 0) {
            std::cout << "connection closed by server\n";
            break;
        }

        if ((fds[0].revents & POLLIN) != 0) {
            char buf[4096];
            const ssize_t got = ::recv(client.fd, buf, sizeof(buf), 0);
            if (got <= 0) {
                std::cout << "connection closed by server\n";
                break;
            }
            client.rx.append(buf, static_cast<std::size_t>(got));
            client.drain();
        }

        if (stdin_open && (fds[1].revents & POLLIN) != 0) {
            // 用裸 read 而不是 std::getline：
            // std::cin 会在用户态缓冲里囤数据，pipe 里读空之后 poll 就不再
            // 报告可读，而剩下的几行还卡在缓冲里——脚本模式会因此挂死。
            char inbuf[4096];
            const ssize_t got = ::read(0, inbuf, sizeof(inbuf));
            if (got <= 0) {
                stdin_open = false;  // EOF：脚本模式跑到这里就只剩收消息了
                ::shutdown(client.fd, SHUT_WR);
                continue;
            }
            pending.append(inbuf, static_cast<std::size_t>(got));

            bool should_quit = false;
            std::size_t pos = 0;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (handle_line(client, line)) {
                    should_quit = true;
                    break;
                }
            }
            if (should_quit) {
                // 不要立刻退出：服务器可能还有最后几条消息（bye、其他人的
                // 告别）在路上。停止读 stdin，继续把 socket 读干净，
                // 直到服务器关闭连接或者空闲超时。
                stdin_open = false;
                ::shutdown(client.fd, SHUT_WR);
            }
        }

        if (!stdin_open) {
            // 脚本模式下给服务器一点时间把最后的回包发回来
            static int idle = 0;
            if (++idle > 4) {
                break;
            }
        }
    }

    ::close(client.fd);
    return 0;
}
