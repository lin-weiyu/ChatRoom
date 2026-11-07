#pragma once

#include <string>

namespace chatroom::platform {

// 把 errno 的值转成可读文本。
//
// 不用裸 strerror() 的两个原因：
//   1. 它返回指向静态缓冲区的指针，多线程下会被覆盖。
//   2. 直接返回 const char* 容易在日志里被误用。
std::string errno_text(int err);

}  // namespace chatroom::platform
