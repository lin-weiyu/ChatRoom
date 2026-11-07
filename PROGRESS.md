# ChatRoom — Project Progress

> 最新状态看这里。完整历史看 `git log --graph`，设计动机看 `docs/adr/`。

## Current Status

- **版本**：v1.0.0
- **最后更新**：2026-11-07
- **一句话**：一个能跑、能测、能压测、有完整决策记录的多客户端聊天服务器

## 里程碑

| # | 里程碑 | 分支 | 状态 |
|---|---|---|---|
| M0 | 工程骨架 / Docker / CI | `chore/dev-env` | ✅ 2026-11-07 |
| M0.5 | Echo server 正确性修复（分层重写） | `fix/echo-correctness` | ✅ |
| M1 | 长度前缀协议 + 编解码 | `feat/protocol-framing` | ✅ |
| M2–M4 | 会话、业务、装配、CLI 客户端 | `feat/chat-core` | ✅ |
| M5 | 健壮性：日志 / 超时 / 连接上限 / 优雅关闭 | `feat/robustness` | ✅ |
| M6 | 线程池 + eventfd | `feat/thread-pool` | ✅ |
| M7 | 集成测试 + 压测 | `test/integration-load` | ✅ |
| M8 | WebSocket + 网页前端 | — | ❌ 未做 |
| M9 | 发布 v1.0.0 | `release/v1.0` | ✅ |

## 实测数据

```
测试：41 个用例全绿（单元 37 + 集成 4）
压测：200 连接 × 20 条消息 → ~438,000 msg/s，0 失败
镜像：两阶段构建，runtime ~80 MB
```

## 架构现状

```
platform -> net -> protocol -> chat -> app
```

| 层 | 文件 | 是否依赖 Linux |
|---|---|---|
| platform | `unique_fd` `sys_error` | 部分（`unistd.h`） |
| net | `buffer` `poller` `acceptor` `connection` `server` `socket` | 是（epoll） |
| protocol | `frame_codec` | 否 |
| chat | `chat_hub` | 否 |
| app | `chat_server` | 是 |
| infra | `logger` `config` `thread_pool` `event_fd` | `event_fd` 是 |

## 已知问题 / 待办

1. **没有 WebSocket 和网页前端**（原计划 M8）。要做的是：HTTP Upgrade 握手
   （需要 SHA1 + base64）、WebSocket 帧解析（掩码、分片、控制帧）、一个静态
   页面。预计 2–3 小时。
2. 消息不持久化，重启即丢。
3. 没有鉴权、没有 TLS、没有限流（连接数上限有，消息速率没有）。
4. `Poller::add` 在 `EEXIST` 时静默失败，目前靠调用顺序保证不会发生；
   应该加断言或错误返回。
5. 工作线程池用一把粗粒度锁；连接数上万时值得改成按房间分片。

## 环境

| 项 | 状态 |
|---|---|
| WSL2 + Ubuntu 24.04.5 | ✅ |
| gcc 13.3 / cmake 3.28 / ninja 1.11 / clang-format 18 | ✅ |
| Docker 29（镜像加速 daocloud） | ✅ |
| GitHub `git@github.com:lin-weiyu/ChatRoom.git` | ✅ |

## 下一步

1. M8：WebSocket + 网页（把协议层复用到 WS 上，验证分层是否真的解耦）
2. 消息持久化（SQLite 或追加日志）
3. 消息速率限流
4. `git bisect` 演练：故意引一个 bug 到历史里，再从历史中定位
