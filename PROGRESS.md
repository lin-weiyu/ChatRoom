# ChatRoom — Project Progress

## Current Status

- **Phase**: Phase 2: Multi-client epoll ✅ 完成
- **Last Updated**: 2026-08-05
- **Current Focus**: 进入 Phase 3 — 自定义聊天协议

## Feature Progress

| Feature | Status | Completed Date | Notes |
|---------|--------|---------------|-------|
| Project structure & CMakeLists | ✅ | 2026-08-05 | - |
| socket → bind → listen → accept | ✅ | 2026-08-05 | - |
| recv / send Echo | ✅ | 2026-08-05 | nc 测试通过 |
| epoll 多客户端 | ✅ | 2026-08-05 | 多个 nc 同时连接，各自正常收发 |
| Phase 3: Chat protocol | 📋 | - | 下一步 |
| Phase 4: Multi-threading | 📋 | - | - |
| Phase 5: Web frontend | 📋 | - | - |

## Known Issues

- 当前还是 Echo（原样返回），不是真正的聊天功能
- 代码全部在 main() 里，功能多了之后需要拆类

## Next Steps

1. 设计简单的聊天协议（消息格式）
2. 实现"一个客户端发消息，其他所有客户端都能收到"（广播）
3. 给客户端分配名字/ID

## Learning Notes

- 用户完成了 epoll 多客户端改造：epoll_create1 → epoll_ctl → epoll_wait → 事件循环
- 理解了 fd 作为"内核对象句柄"的 C 语言设计模式
- 理解了 listen() 的语义：不是配置，是"开门营业"的开关
- 理解了 epoll 原理：内核主动推送就绪 fd，不是遍历扫描
