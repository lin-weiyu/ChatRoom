#include "chatroom/infra/logger.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace chatroom::infra {

namespace {

std::mutex g_mutex;
LogLevel g_level = LogLevel::kInfo;

const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::kDebug:
            return "DEBUG";
        case LogLevel::kInfo:
            return "INFO ";
        case LogLevel::kWarn:
            return "WARN ";
        case LogLevel::kError:
            return "ERROR";
    }
    return "?????";
}

}  // namespace

void set_log_level(LogLevel level) {
    const std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
}

LogLevel log_level() {
    const std::lock_guard<std::mutex> lock(g_mutex);
    return g_level;
}

void log(LogLevel level, std::string_view message) {
    const std::lock_guard<std::mutex> lock(g_mutex);
    if (level < g_level) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);

    const long ms = static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() %
        1000);

    std::fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld] %s %.*s\n", tm_buf.tm_year + 1900,
                 tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                 ms, level_name(level), static_cast<int>(message.size()), message.data());
}

void log_debug(std::string_view message) { log(LogLevel::kDebug, message); }
void log_info(std::string_view message) { log(LogLevel::kInfo, message); }
void log_warn(std::string_view message) { log(LogLevel::kWarn, message); }
void log_error(std::string_view message) { log(LogLevel::kError, message); }

}  // namespace chatroom::infra
