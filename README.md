# ChatRoom

一个从零实现的多客户端 TCP 聊天服务器，用 C++17 写成，**不用任何网络库**——
socket、epoll、缓冲区、协议解析全部手写。

这个仓库同时是教材：提交历史、PR 记录、ADR 都是可读的学习材料。

## 当前状态

**M0 — 工程骨架**。构建、测试、容器、CI 已就位，通信功能从 M1 开始。

## 快速开始

### Linux / WSL2（推荐，构建最快）

```bash
./scripts/build.sh          # Debug 构建 + 跑测试
./build/chatroom_server
```

### Docker（任何平台，环境与 CI 完全一致）

```bash
./scripts/run.sh            # 起服务器，宿主机 12345 端口
./scripts/dev.sh            # 只构建 + 测试，不进容器
```

### Windows 上首次使用

本项目依赖 Linux 的 `epoll`，Windows 需要 WSL2：

```powershell
wsl --install -d Ubuntu-24.04
```

仓库留在 Windows 盘，Linux 里通过 `/mnt/e/github-repository` 访问。

## 目录结构

```
include/chatroom/     公共头文件
  platform/           系统调用封装（fd、errno、地址）
  net/                事件循环、连接、缓冲区
  protocol/           帧编解码
  chat/               业务：用户、房间、消息路由
src/                  实现
tests/unit/           单元测试（不需要网络）
tests/integration/    集成测试（真 socket）
tools/                命令行客户端
docs/adr/             架构决策记录
docs/pr/              每个 PR 的设计说明
docs/journal/         学习日志
```

## 架构

```
chat -> protocol -> net -> platform
```

依赖只能向下、且只能相邻。业务层不认识 socket，网络层不认识聊天。
这条规则由 CMake 的 target 链接关系强制。

详见 [`docs/adr/0001-layered-architecture.md`](docs/adr/0001-layered-architecture.md)。

## 开发约定

见 [`CONTRIBUTING.md`](CONTRIBUTING.md)：分支命名、提交格式、PR 流程。

## 技术栈

C++17 · CMake · Ninja · GoogleTest · Docker · GitHub Actions · epoll
