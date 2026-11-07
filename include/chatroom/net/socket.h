#pragma once

#include <cstdint>

#include "chatroom/platform/unique_fd.h"

namespace chatroom::net {

// 创建监听 socket 并进入 listen 状态。
//
// 内部按固定顺序做四件事：socket -> setsockopt(SO_REUSEADDR) -> bind -> listen，
// 然后把 fd 设成非阻塞。端口传 0 时由内核分配，真实端口用 local_port() 查询
// （集成测试用得上）。
//
// 失败时返回无效 UniqueFd，errno 保持调用现场的值。
platform::UniqueFd create_listener(std::uint16_t port, int backlog = 128);

// 查询已绑定 socket 的实际端口
std::uint16_t local_port(int fd);

void set_nonblocking(int fd);

void set_tcp_nodelay(int fd);

}  // namespace chatroom::net
