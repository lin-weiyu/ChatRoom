# ChatRoom

[![CI](https://github.com/lin-weiyu/ChatRoom/actions/workflows/ci.yml/badge.svg)](https://github.com/lin-weiyu/ChatRoom/actions/workflows/ci.yml)

一个从零实现的多客户端 TCP 聊天服务器，C++17，**不用任何网络库**——socket、
epoll、缓冲区、协议解析、线程池全部手写。

这个仓库同时是教材：提交历史、ADR、PR 记录都是可读的学习材料。

## 功能

- 长度前缀二进制协议（自定义，非 HTTP/WebSocket）
- 昵称、房间、广播、私聊
- 命令：`/join` `/nick` `/msg` `/dm` `/joinroom` `/leaveroom` `/list` `/quit`
- 非阻塞 I/O + epoll，支持数千并发连接
- 空闲超时、连接数上限、协议错误即断开
- IO 线程 + 工作线程池（可 `--workers 0` 退回单线程）
- 优雅关闭（SIGINT）
- 单元测试 + 进程内集成测试 + 压测工具

## 快速开始

```bash
# 构建 + 测试（Linux / WSL2）
./scripts/build.sh

# 起服务器
./build/chatroom_server --port 12345

# 另一个终端：连上去聊
./build/chatroom_cli 127.0.0.1 12345
> /join alice
> /msg hello world
> /list
> /quit
```

### Docker（任何平台）

```bash
./scripts/dev.sh     # 在容器里构建 + 测试（与 CI 同环境）
./scripts/run.sh     # 起服务器，宿主 12345 可连
```

### Windows

项目依赖 Linux 的 `epoll`，Windows 需要 WSL2：

```powershell
wsl --install -d Ubuntu-24.04
```

仓库留在 Windows 盘，Linux 里通过 `/mnt/e/github-repository` 访问。

## 命令行参数

```
--port <n>          监听端口（0 = 内核分配）
--max-conns <n>     连接数上限，超出直接拒绝
--idle-timeout <s>  空闲多久断开（0 = 关闭）
--workers <n>       业务线程数（0 = 全部在 IO 线程执行）
--backlog <n>       listen backlog
--verbose           打开 DEBUG 日志
```

## 架构

```
chat      业务：用户、房间、消息路由        不认识 socket
protocol  Wire 格式：Frame / FrameCodec     纯函数
net       epoll、连接、缓冲区               只认识字节流
platform  fd / errno 封装
```

依赖只能向下、且只能相邻。违反分层会在**链接期**失败——这条规则由 CMake 的
target 关系强制，不靠文档自觉。

线程模型：

```
IO 线程（1）  epoll / recv / send / 连接生死
     │ 投递帧（只传值）
     ▼
工作线程池（N） ChatHub::handle → Outbound
     │ 入队 + eventfd 唤醒
     ▼
IO 线程 派发 send
```

详见 [`docs/adr/0003-threading-model.md`](docs/adr/0003-threading-model.md)。

## 性能

`./scripts/bench.sh 200 20`（200 连接 × 20 条消息，本机 WSL2）：

```
sent: 4000, failed: 0
throughput: ~438000 msg/s
```

## 目录

```
include/chatroom/   公共头文件（按层分目录）
src/                实现
tests/unit/         单元测试（不需要网络）
tests/integration/  集成测试（进程内起真实服务器）
tools/chat_cli      调试客户端
tools/loadgen       压测工具
docs/adr/           架构决策记录
docs/protocol.md    协议规范
scripts/            build / smoke / bench
```

## 文档

| 文档 | 内容 |
|---|---|
| [`docs/protocol.md`](docs/protocol.md) | 协议规范（唯一真相） |
| [`docs/adr/0001-layered-architecture.md`](docs/adr/0001-layered-architecture.md) | 为什么分层 |
| [`docs/adr/0002-wire-protocol.md`](docs/adr/0002-wire-protocol.md) | 为什么用长度前缀 |
| [`docs/adr/0003-threading-model.md`](docs/adr/0003-threading-model.md) | 为什么用线程池 |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | 分支、提交、PR 规范 |
| [`PROGRESS.md`](PROGRESS.md) | 进度与已知问题 |

## 技术栈

C++17 · CMake · Ninja · GoogleTest · epoll · pthread · Docker · GitHub Actions
