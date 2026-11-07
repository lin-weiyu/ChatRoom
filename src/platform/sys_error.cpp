#include "chatroom/platform/sys_error.h"

#include <system_error>

namespace chatroom::platform {

std::string errno_text(int err) {
    // generic_category() 对应 C 标准库/POSIX 的 errno 语义，
    // 和 system_category()（Windows 的 Win32 错误码）区分开。
    return std::generic_category().message(err);
}

}  // namespace chatroom::platform
