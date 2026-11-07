#pragma once

#include <string_view>

namespace chatroom::infra {

enum class LogLevel : int {
    kDebug = 0,
    kInfo = 1,
    kWarn = 2,
    kError = 3,
};

void set_log_level(LogLevel level);

LogLevel log_level();

// 线程安全的日志输出。
//
// 为什么不用 printf：多个线程同时写 stderr 会把行撕开，日志变成
// 半条 A 半条 B 的乱码。这里用一把锁保证"一行是一次原子写"。
//
// 为什么暂时不引 spdlog：M3 的目标是理解"日志被谁、在什么上下文里写"，
// 引一个功能齐全的库会把这层遮住。等 M6 线程池落地之后，如果发现
// 性能真的有问题，再换也不迟。
void log(LogLevel level, std::string_view message);

void log_debug(std::string_view message);
void log_info(std::string_view message);
void log_warn(std::string_view message);
void log_error(std::string_view message);

}  // namespace chatroom::infra
