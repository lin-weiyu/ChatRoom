# ChatRoom — Learning Plan

## Skill Level Assessment

- **当前水平**: ACM ICPC 金银牌，C++ 算法能力强
- **已掌握**: C++17、CMake、设计模式、GTest、CI
- **已完成项目**: mini-git、todo-cli
- **待提升**: 网络编程、并发编程、项目文档规范

## Learning Roadmap

### Phase 1 — Echo Server（当前）

| 需要学习 | 资源 |
|----------|------|
| Socket API (socket/bind/listen/accept) | [Beej's Guide](https://beej.us/guide/bgnet/) Ch1-6 |
| TCP 连接生命周期 | Beej's Guide Ch2 |
| 字节序 (htons/htonl) | `man byteorder` |
| send/recv 数据收发 | Beej's Guide Ch5-6 |
| 错误处理 (errno) | `man errno` |

### Phase 2 — Multi-client (epoll)

| 需要学习 | 资源 |
|----------|------|
| I/O 多路复用概念 | [The C10K Problem](http://www.kegel.com/c10k.html) |
| epoll API | `man 7 epoll` |
| 非阻塞 I/O | `man 2 fcntl` |
| 粘包处理 | - |

### Phase 3 — Chat Protocol

| 需要学习 | 资源 |
|----------|------|
| 应用层协议设计 | - |
| 消息解析 | - |

### Phase 4 — Multi-threading

| 需要学习 | 资源 |
|----------|------|
| std::thread + mutex | cppreference |
| 线程池模式 | - |

### Phase 5 — Web Access

| 需要学习 | 资源 |
|----------|------|
| HTTP 基础 | MDN |
| WebSocket | RFC 6455 |
| 基础 HTML/CSS/JS | MDN |

## Adjustment Log

- **2026-08-02**: 初始计划。基于 mini-git 和 todo-cli 分析：网络编程零基础，从 Echo Server 起步。
