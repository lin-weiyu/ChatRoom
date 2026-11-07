// 压测工具：N 个连接，每个连接发 M 条消息。
//
//   ./build/chatroom_loadgen 127.0.0.1 12345 200 50
//
// 输出总吞吐和每个连接的延迟分布（粗略）。目标不是做一个专业压测工具，
// 而是让你能量化"单线程 reactor 到底能扛多少"。

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "chatroom/protocol/frame_codec.h"

namespace {

using chatroom::protocol::Frame;
using chatroom::protocol::MessageType;

bool connect_to(const std::string& host, std::uint16_t port, int& fd) {
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

void drain(int fd) {
    char buf[8192];
    while (::recv(fd, buf, sizeof(buf), MSG_DONTWAIT) > 0) {
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::uint16_t port = 12345;
    int connections = 50;
    int messages = 20;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<std::uint16_t>(std::atoi(argv[2]));
    if (argc > 3) connections = std::atoi(argv[3]);
    if (argc > 4) messages = std::atoi(argv[4]);

    if (connections <= 0 || messages <= 0) {
        std::cerr << "usage: " << argv[0] << " [host] [port] [connections] [messages]\n";
        return 2;
    }

    std::atomic<long> sent{0};
    std::atomic<long> failed{0};

    const auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(connections));

    for (int i = 0; i < connections; ++i) {
        threads.emplace_back([&, i] {
            int fd = -1;
            if (!connect_to(host, port, fd)) {
                failed.fetch_add(messages);
                return;
            }

            const std::string nickname = "user" + std::to_string(i);
            Frame join;
            join.type = MessageType::kJoin;
            join.fields = {nickname};
            const std::string join_bytes = chatroom::protocol::encode(join);
            ::send(fd, join_bytes.data(), join_bytes.size(), MSG_NOSIGNAL);

            for (int m = 0; m < messages; ++m) {
                Frame msg;
                msg.type = MessageType::kMsg;
                msg.fields = {"", "load-message-" + std::to_string(m)};
                const std::string bytes = chatroom::protocol::encode(msg);

                const ssize_t n = ::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                if (n == static_cast<ssize_t>(bytes.size())) {
                    sent.fetch_add(1, std::memory_order_relaxed);
                } else {
                    failed.fetch_add(1, std::memory_order_relaxed);
                }

                // 不读的话，服务端的广播会很快把我们的接收缓冲塞满，
                // 反过来阻塞服务端的 send —— 那测到的就是背压而不是吞吐。
                drain(fd);
            }

            ::close(fd);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start);
    const long total = sent.load();

    std::cout << "connections: " << connections << ", messages/conn: " << messages << "\n";
    std::cout << "sent: " << total << ", failed: " << failed.load() << "\n";
    std::cout << "elapsed: " << elapsed.count() << " s\n";
    if (elapsed.count() > 0) {
        std::cout << "throughput: " << static_cast<long>(static_cast<double>(total) / elapsed.count())
                  << " msg/s\n";
    }
    return 0;
}
