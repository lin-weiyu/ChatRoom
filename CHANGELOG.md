# Changelog

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [1.0.0] - 2026-11-07

首个可用版本。

### Added

- **协议**：长度前缀二进制帧（`[4B len][1B type][字段]`），上限 64 KiB
- **业务**：昵称、房间、广播、私聊；命令 `/join` `/nick` `/msg` `/dm`
  `/joinroom` `/leaveroom` `/list` `/quit` `/ping`
- **网络**：非阻塞 socket + epoll、读写缓冲、部分写处理、半关闭、
  `EPOLLHUP/ERR` 处理
- **并发**：固定大小线程池 + eventfd 跨线程唤醒；`--workers 0` 可退回单线程
- **健壮性**：空闲超时、连接数上限、协议错误断开、SIGINT 优雅关闭
- **基础设施**：分级日志、命令行配置、RAII fd 封装
- **测试**：41 个用例（单元 + 进程内集成），覆盖半包/粘包/超长帧/截断字段
- **工具**：`chat_cli` 调试客户端、`loadgen` 压测工具
- **工程**：CMake 分 target、Docker 两阶段镜像、GitHub Actions（Linux/macOS/
  format/docker 四个 job）、clang-format 强制

### Changed

- 默认分支由 `master` 更名为 `main`

### Removed

- 过程式的 `src/main_server.cpp`（内容存档于提交 `f1a5d6c`）

### Known limitations

- 没有 WebSocket / 网页前端（原计划的 M8）
- 消息不持久化，重启即丢
- 没有鉴权与 TLS

[1.0.0]: https://github.com/lin-weiyu/ChatRoom/releases/tag/v1.0.0
