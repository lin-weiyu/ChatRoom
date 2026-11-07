// version.h — 版本信息
//
// 所属层：core（最底层的公共信息）
// 被谁使用：main.cpp（启动日志）、--version 参数
//
// 版本号只有一个来源：CMakeLists.txt 的 project(VERSION ...)。
// 通过编译定义注入，不在源码里硬编码，避免两处不一致。

#pragma once

#include <string_view>

namespace chatroom {

/// 形如 "1.0.1"。
std::string_view version() noexcept;

/// "Debug" 或 "Release"，由构建类型决定。用于启动日志。
std::string_view build_type() noexcept;

}  // namespace chatroom
