// sys_error.h — errno 的可读化
//
// 所属层：platform
// 被谁使用：任何需要把系统调用失败原因写进日志的地方
//
// 背景：系统调用失败时返回 -1，真正的错误码放在 errno 里。
// errno 是个"公共格子"——它属于线程而不是属于某次调用，任何后续系统调用
// 都可能覆盖它。所以要么立刻转成字符串，要么立刻存下来。

#pragma once

#include <string>

namespace chatroom::platform {

/// 把 errno 的值转成可读文本。
///
/// @param err  通常传 errno 的当前值（调用方应立刻读取，不要隔几条语句）
/// @return     形如 "Resource temporarily unavailable" 的描述
///
/// 不用裸 strerror() 的两个原因：
///   1. 它返回指向静态缓冲区的指针，多线程下会被互相覆盖
///   2. 直接返回 const char* 容易在日志里被误用（比如被当成格式化串）
std::string errno_text(int err);

}  // namespace chatroom::platform
