#include <csignal>
#include <cstdlib>
#include <iostream>

#include "chatroom/app/chat_server.h"
#include "chatroom/version.h"

namespace {

// 信号处理函数里只能做"异步信号安全"的操作，所以这里只置一个标志位，
// 真正的退出逻辑留在事件循环里（每轮最多 200ms 就会检查一次）。
volatile std::sig_atomic_t g_stop = 0;

void handle_signal(int) { g_stop = 1; }

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // 兜底：即使某处漏了 MSG_NOSIGNAL，也不至于被一个已关闭的连接杀掉进程。
    std::signal(SIGPIPE, SIG_IGN);

    // 端口可通过参数指定；传 0 让内核分配（测试用）。
    std::uint16_t port = 12345;
    if (argc > 1) {
        port = static_cast<std::uint16_t>(std::atoi(argv[1]));
    }

    chatroom::app::ChatServer server(port);
    if (!server.valid()) {
        std::cerr << "failed to listen on port " << port << std::endl;
        return 1;
    }

    // endl 而不是 "\n"：stdout 重定向到管道时是全缓冲的，
    // 不刷新的话，进程被 kill 之前这行日志根本不会出现在日志文件里。
    std::cout << "chatroom " << chatroom::version() << " listening on port " << server.port()
              << std::endl;

    while (g_stop == 0) {
        server.run_once();
    }

    std::cout << "shutdown" << std::endl;
    return 0;
}
