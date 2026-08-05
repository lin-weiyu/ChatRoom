# ChatRoom — Project Progress

## Current Status

- **Phase**: 规划中 — Phase 1: Echo Server
- **Last Updated**: 2026-08-02
- **Current Focus**: 用户开始学习 Linux Socket 编程，准备实现 Echo Server

## Feature Progress

| Feature | Status | Completed Date | Notes |
|---------|--------|---------------|-------|
| Project scaffold (CMake, CI, GTest) | ✅ | 2026-08-02 | 初始提交 |
| Phase 1: Echo Server (单连接) | 📋 | - | 待开始 |
| Phase 2: Multi-client (epoll) | 📋 | - | - |
| Phase 3: Chat protocol + rooms | 📋 | - | - |
| Phase 4: Multi-threading | 📋 | - | - |
| Phase 5: Web frontend | 📋 | - | - |

## Known Issues

- 暂无（项目刚开始）

## Next Steps

1. 阅读 Beej's Guide to Network Programming 前 5 章
2. 实现 TCP Echo Server（socket → bind → listen → accept → recv → send）
3. 用 `nc` 或 `telnet` 手动测试
4. 补充单元测试
