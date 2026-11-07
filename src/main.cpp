#include "chatroom/app/chat_server.h"
#include "chatroom/infra/config.h"
#include "chatroom/infra/logger.h"
#include "chatroom/version.h"

#include <csignal>
#include <cstdlib>
#include <iostream>

namespace {

// 信号处理函数里只能做"异步信号安全"的操作，所以这里只置一个标志位，
// 真正的退出逻辑留在事件循环里（每轮最多 200ms 就会检查一次）。
volatile std::sig_atomic_t g_stop = 0;

void handle_signal(int) {
    g_stop = 1;
}

}  // namespace

int main(int argc, char** argv) {
    const auto parsed = chatroom::infra::parse_args(argc, argv);
    if (!parsed.ok) {
        std::cerr << parsed.message << "\n\n" << chatroom::infra::usage(argv[0]);
        return 2;
    }
    if (parsed.should_exit) {
        std::cout << parsed.message << "\n";
        return 0;
    }

    if (parsed.config.verbose) {
        chatroom::infra::set_log_level(chatroom::infra::LogLevel::kDebug);
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // 兜底：即使某处漏了 MSG_NOSIGNAL，也不至于被一个已关闭的连接杀掉进程。
    std::signal(SIGPIPE, SIG_IGN);

    chatroom::app::ChatServer server(parsed.config);
    if (!server.valid()) {
        chatroom::infra::log_error("failed to listen on port " +
                                   std::to_string(parsed.config.port));
        return 1;
    }

    chatroom::infra::log_info(std::string("chatroom ") + std::string(chatroom::version()) +
                              " listening on port " + std::to_string(server.port()));

    while (g_stop == 0) {
        server.run_once();
    }

    server.shutdown();
    // 给内核一点时间把 FIN 和最后的消息发出去
    for (int i = 0; i < 3; ++i) {
        server.run_once(100);
    }

    chatroom::infra::log_info("bye");
    return 0;
}
