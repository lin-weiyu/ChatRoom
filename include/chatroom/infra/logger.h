// logger.h — 分级日志
//
// 所属层：infra（平台无关，谁都能用）
// 被谁使用：主要在 app 层
//
// 为什么自己写而不引 spdlog：这个项目的目标是理解"日志被谁、在什么上下文
// 里写"。引一个功能齐全的库会把这层遮住。等 M6 线程池落地后如果发现性能
// 真的有问题，再换也不迟。

#pragma once

#include <string_view>

namespace chatroom::infra {

enum class LogLevel : int {
    kDebug = 0,
    kInfo = 1,
    kWarn = 2,
    kError = 3,
};

/// 设置最低输出级别。低于该级别的日志被丢弃。
void set_log_level(LogLevel level);

LogLevel log_level();

/// 线程安全地输出一行日志。
///
/// 为什么必须加锁：多个线程同时写 stderr 会把行撕开，日志变成
/// "半条 A 半条 B" 的乱码。一把锁保证"一行 = 一次原子写"。
///
/// @param message 不含换行符；输出时会自动加时间戳、级别和换行
void log(LogLevel level, std::string_view message);

void log_debug(std::string_view message);
void log_info(std::string_view message);
void log_warn(std::string_view message);
void log_error(std::string_view message);

}  // namespace chatroom::infra
