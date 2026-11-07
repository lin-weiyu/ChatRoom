# ChatRoom — Learning Plan

## 起点评估

- **已有**：ACM 选手，C++ 算法与数据结构能力强，C++17 基础扎实
- **缺少**：TCP 与 Linux 系统调用、epoll 之外的 I/O 模型、并发与线程、工程化流程
- **目标**：一个能写进简历的完整项目 + 能对着代码讲清每一层的工程能力

## 学习路线（与里程碑对齐）

### 已完成

| 阶段 | 内容 | 产出 |
|---|---|---|
| Phase 1 | socket / bind / listen / accept / send / recv | echo server |
| Phase 2 | epoll 多客户端 | 多连接 echo |
| M0 | 工程骨架：CMake target、Docker、CI、协作规范 | 可构建可测试可交付的仓库 |

### 进行中

| 里程碑 | 学什么 | 关键问题 |
|---|---|---|
| M0.5 | 非阻塞 I/O、部分写、SIGPIPE、RAII | 为什么 `send` 返回值不能忽略？ |
| M1 | TCP 字节流、消息边界、大端序 | 粘包到底是谁造成的？ |
| M2 | fd 生命周期、ET vs LT、EPOLLOUT | 什么时候该注册 EPOLLOUT？ |

### 后续

| 里程碑 | 学什么 |
|---|---|
| M3 | 广播扇出、背压、慢客户端 |
| M4 | 业务状态与连接状态分离 |
| M5 | 心跳超时、限流、优雅关闭 |
| M6 | 线程模型、竞态、mutex/condvar、TSan |
| M7 | 可测试性设计、集成测试、压测 |
| M8 | HTTP / WebSocket（RFC 6455） |
| M9 | 语义化版本、发布流程 |

## 推荐资料

| 主题 | 资料 |
|---|---|
| Socket 编程总览 | [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) |
| epoll | `man 7 epoll`、`man 2 epoll_wait` |
| 非阻塞 I/O | `man 2 fcntl`、`man 7 socket` |
| C10K 问题 | [The C10K Problem](http://www.kegel.com/c10k.html) |
| 服务端设计 | 陈硕《Linux 多线程服务端编程》（muduo 作者，只读不引） |
| WebSocket | RFC 6455、MDN |
| C++ 并发 | 《C++ Concurrency in Action》 |

## 调整记录

- **2026-08-02**：初始计划，从 Echo Server 起步
- **2026-08-10**：Phase 2 完成，转向聊天协议
- **2026-11-07**：改为里程碑制（M0–M9），引入 Docker 与 AI 建设期/教学期双轨模式；
  确认架构分层与长度前缀协议
