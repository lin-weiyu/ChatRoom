#pragma once

#include <cstdint>
#include <string>

namespace chatroom::infra {

// 运行参数。
//
// 全部来自命令行，不读配置文件：这个项目的目标是讲清楚网络编程，
// 配置系统本身不是学习目标。等 M9 有需要再引入 TOML/JSON。
struct Config {
    std::uint16_t port = 12345;
    int backlog = 128;
    int max_connections = 1024;
    int idle_timeout_seconds = 120;  // 0 表示不超时
    int worker_threads = 2;          // 0 表示所有业务在 IO 线程里同步执行
    bool verbose = false;
};

struct ParseOutcome {
    bool ok = false;
    bool should_exit = false;  // --help / --version
    Config config;
    std::string message;
};

ParseOutcome parse_args(int argc, char** argv);

std::string usage(const char* program);

}  // namespace chatroom::infra
