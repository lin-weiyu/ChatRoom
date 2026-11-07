// config.h — 运行参数
//
// 所属层：infra
// 被谁使用：main.cpp（解析）、app::ChatServer（读取行为参数）
//
// 全部来自命令行，不读配置文件：这个项目的目标是讲清楚网络编程，
// 配置系统本身不是学习目标。

#pragma once

#include <cstdint>
#include <string>

namespace chatroom::infra {

/// 服务器运行参数。
struct Config {
    std::uint16_t port = 12345;      ///< 监听端口，0 表示由内核分配
    int backlog = 128;               ///< listen 的全连接队列长度
    int max_connections = 1024;      ///< 0 表示不限制
    int idle_timeout_seconds = 120;  ///< 0 表示不超时
    int worker_threads = 2;          ///< 0 表示全部业务在 IO 线程执行
    bool verbose = false;            ///< 打开 DEBUG 日志
};

/// 命令行解析结果。
struct ParseOutcome {
    bool ok = false;           ///< 解析是否成功
    bool should_exit = false;  ///< true 表示 --help/--version，直接退出
    Config config;
    std::string message;  ///< 错误信息，或 --help/--version 的输出
};

/// 解析 argv。成功返回 ok=true；参数错误返回 ok=false 并填好 message。
ParseOutcome parse_args(int argc, char** argv);

/// 生成 --help 文本。
std::string usage(const char* program);

}  // namespace chatroom::infra
