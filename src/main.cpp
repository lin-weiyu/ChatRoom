#include <csignal>
#include <iostream>
#include <string_view>

#include "chatroom/net/server.h"
#include "chatroom/version.h"

namespace {

// 信号处理函数里只能做"异步信号安全"的操作，所以这里只置一个标志位，
// 真正的退出逻辑留在事件循环里（每轮最多 200ms 就会检查一次）。
volatile std::sig_atomic_t g_stop = 0;

void handle_signal(int) { g_stop = 1; }

// M0.5 的行为：原样回发。
//
// 注意这段代码在 main（装配层），不在 net 层——net 只负责"把字节搬进来、
// 把字节搬出去"，"搬进来之后干什么"是上层的策略。M3 会把这里换成
// 协议解析 + 广播。
void echo_handler(chatroom::net::Connection& conn, std::string_view data) {
    conn.send(data);
    conn.input().retrieve(data.size());
}

}  // namespace

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // 兜底：即使某处漏了 MSG_NOSIGNAL，也不至于被一个已关闭的连接杀掉进程。
    std::signal(SIGPIPE, SIG_IGN);

    chatroom::net::Server server(12345);
    if (!server.valid()) {
        std::cerr << "failed to listen on port 12345" << std::endl;
        return 1;
    }

    server.set_message_handler(echo_handler);

    // endl 而不是 "\n"：stdout 重定向到管道时是全缓冲的，
    // 不刷新的话，进程被 kill 之前这行日志根本不会出现在日志文件里。
    std::cout << "chatroom " << chatroom::version() << " listening on port " << server.port()
              << " (echo mode)" << std::endl;

    while (g_stop == 0) {
        server.run_once();
    }

    std::cout << "shutdown" << std::endl;
    return 0;
}
