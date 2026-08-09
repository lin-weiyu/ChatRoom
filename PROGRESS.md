# ChatRoom — Project Progress

## Current Status

- **Phase**: Phase 1: Echo Server ✅ 完成
- **Last Updated**: 2026-08-05
- **Current Focus**: 进入 Phase 2 — 多客户端支持（epoll）

## Feature Progress

| Feature | Status | Completed Date | Notes |
|---------|--------|---------------|-------|
| Project structure & CMakeLists | ✅ | 2026-08-05 | cmake 配置通过 |
| socket → bind → listen → accept | ✅ | 2026-08-05 | 单次连接正常 |
| recv / send Echo | ✅ | 2026-08-05 | nc 测试通过，收发正常 |
| Phase 2: Multi-client (epoll) | 📋 | - | 下一步 |
| Phase 3: Chat protocol + rooms | 📋 | - | - |
| Phase 4: Multi-threading | 📋 | - | - |
| Phase 5: Web frontend | 📋 | - | - |

## Known Issues

- 当前版只能服务一个客户端（accept 一次就退出），多客户端需要 epoll

## Next Steps

1. 理解"为什么一个服务器要同时处理多个客户端"——阻塞 I/O 的局限
2. 学习 epoll 三个 API：`epoll_create1` / `epoll_ctl` / `epoll_wait`
3. 用 epoll 改造成多客户端 Echo Server
4. 多个 nc 终端同时连接测试

## Learning Notes

- 用户完成了完整的 TCP Echo Server：socket → bind → listen → accept → recv → send → close
- 踩了 Ctrl+Z vs Ctrl+C 的坑，学习了进程管理命令（ps/pgrep/ss/kill/pkill/killall）
