# ChatRoom — Project Progress

> 最新状态看这里。详细历史看 `git log --graph`，设计动机看 `docs/adr/`。

## Current Status

- **里程碑**：M0 — 工程骨架 ✅ 完成
- **最后更新**：2026-11-07
- **下一步**：M0.5 — 在新结构上重写 echo server，修掉阻塞 I/O、部分写、SIGPIPE 等缺陷

## 里程碑进度

| # | 里程碑 | 分支 | 状态 | 完成日期 |
|---|---|---|---|---|
| M0 | 工程骨架 / Docker / CI | `chore/dev-env` | ✅ | 2026-11-07 |
| M0.5 | Echo server 正确性修复 | `fix/echo-correctness` | 📋 | - |
| M1 | 长度前缀协议 + Parser | `feat/protocol-framing` | 📋 | - |
| M2 | Connection / Buffer / EventLoop | `feat/session-registry` | 📋 | - |
| M3 | ChatHub 广播 + 背压 | `feat/chat-broadcast` | 📋 | - |
| M4 | 房间与私聊 | `feat/rooms-and-dm` | 📋 | - |
| M5 | 健壮性：心跳 / 限流 / 优雅关闭 | `feat/robustness` | 📋 | - |
| M6 | 线程池 + TSan | `feat/thread-pool` | 📋 | - |
| M7 | 集成测试与压测 | `test/integration-load` | 📋 | - |
| M8 | WebSocket + 网页 | `feat/websocket-frontend` | 📋 | - |
| M9 | 发布 v1.0 | `release/v1.0` | 📋 | - |

## 环境

| 项 | 状态 |
|---|---|
| WSL2 + Ubuntu 24.04 | ✅ |
| gcc 13 / cmake 3.28 / ninja 1.11 / gdb / clang-format 18 | ✅ |
| Docker（镜像加速：daocloud） | ✅ |
| GitHub 远端 `git@github.com:lin-weiyu/ChatRoom.git` | ✅ |

## 已知问题

- Docker Hub 直连超时，已通过 `registry-mirrors` 解决（镜像加速依赖第三方可用性）
- 尚未有真正能聊天的功能，`chatroom_server` 目前只打印版本号

## 下一步计划

1. M0.5：`UniqueFd` / `Buffer` / `Connection` 落地，echo server 在分层结构上重写
2. 用 `git show f1a5d6c:src/main_server.cpp` 对照旧实现的缺陷，逐个修
