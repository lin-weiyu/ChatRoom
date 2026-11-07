#pragma once

#include <string_view>

namespace chatroom {

// 版本号由 CMake 注入（见 CMakeLists.txt 的 project(VERSION ...)），
// 不在源码里硬编码，避免两处不一致。
std::string_view version() noexcept;

// 编译期构建类型（Debug / Release），用于启动日志。
std::string_view build_type() noexcept;

}  // namespace chatroom
