// socket.h — socket 创建与属性的薄封装
//
// 所属层：net
// 被谁使用：Acceptor（监听 socket）、Acceptor::accept_all（客户端 fd 属性）
//
// 把这些琐碎但容易漏的 setsockopt 集中在这里，是为了让业务代码里
// 不再出现裸系统调用。

#pragma once

#include "chatroom/platform/unique_fd.h"

#include <cstdint>

namespace chatroom::net {

/// 创建监听 socket：socket → SO_REUSEADDR → bind → listen → 非阻塞。
///
/// @param port     监听端口；传 0 时由内核分配（集成测试用）
/// @param backlog  全连接队列长度上限
/// @return         成功时持有有效 fd；失败时返回无效对象，errno 保留现场值
///
/// 为什么一定要 SO_REUSEADDR：
///   服务重启时，旧连接处于 TIME_WAIT 状态，地址仍被占用。没有这个选项
///   会看到 "Address already in use"，然后白白等 60 秒。
platform::UniqueFd create_listener(std::uint16_t port, int backlog = 128);

/// 查询已绑定 socket 的实际端口。构造时传 0 的情况下用它拿真实端口。
std::uint16_t local_port(int fd);

/// 把 fd 设为非阻塞。
///
/// 这是整套代码正确性的前提：Connection 的读循环、Acceptor 的 accept 循环
/// 都依赖"没有数据时立刻返回 EAGAIN"这个行为。如果忘了调用，服务器会在
/// 第一次"数据不够读"时卡死。
void set_nonblocking(int fd);

/// 关闭 Nagle 算法。
///
/// Nagle 会把小包攒起来一起发，降低网络开销但增加延迟。聊天场景下消息
/// 很小、对延迟敏感，所以关掉。
void set_tcp_nodelay(int fd);

}  // namespace chatroom::net
