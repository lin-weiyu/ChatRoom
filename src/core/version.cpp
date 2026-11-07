#include "chatroom/version.h"

namespace chatroom {

namespace {
// CHATROOM_VERSION 由 CMake 的 project() 通过编译定义注入，
// 见 CMakeLists.txt 中 target_compile_definitions(chatroom_core ...)。
constexpr std::string_view kVersion = CHATROOM_VERSION;
constexpr std::string_view kBuildType = CHATROOM_BUILD_TYPE;
}  // namespace

std::string_view version() noexcept { return kVersion; }

std::string_view build_type() noexcept { return kBuildType; }

}  // namespace chatroom
