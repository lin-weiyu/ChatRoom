# ChatRoom — Project Progress

## Current Status

- **Phase**: Phase 1: Echo Server — 代码待写
- **Last Updated**: 2026-08-05
- **Current Focus**: 在 main_server.cpp 实现 socket → bind → listen → accept 最小服务器

## Feature Progress

| Feature | Status | Completed Date | Notes |
|---------|--------|---------------|-------|
| Project structure & CMakeLists | ✅ | 2026-08-05 | CMakeLists 拼写已修复，cmake 配置通过 |
| Phase 1: Echo Server (单连接) | 📋 | - | main_server.cpp 仍为空，socket 代码未写 |
| Phase 2: Multi-client (epoll) | 📋 | - | - |
| Phase 3: Chat protocol + rooms | 📋 | - | - |
| Phase 4: Multi-threading | 📋 | - | - |
| Phase 5: Web frontend | 📋 | - | - |

## Known Issues

- [High] main_server.cpp 为空，Echo Server 核心代码未写 (2026-08-05)

## Next Steps

1. 在 main_server.cpp 写 socket() → bind() → listen() → accept() 代码
2. `cmake -B build && cmake --build build` 编译
3. 终端1运行 `./build/chatroom`，终端2用 `nc 127.0.0.1 12345` 测试连接

## Learning Notes

- CMake 命令统一全小写：`cmake_minimum_required` / `project` / `set` / `add_executable`
