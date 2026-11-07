# ADR-0001：分层架构

- 状态：已通过
- 日期：2026-11-07
- 决策者：项目所有者

## 背景

项目从 80 行的 `main()` 起步（`f1a5d6c`），能跑，但不具备任何可维护性：

- 所有逻辑在一个函数里，无法单独测试任何一部分
- 网络细节（fd、epoll）和业务（消息、用户）纠缠在一起
- 想加一个功能就得改 `main()`，改一处影响全局

目标是一个**能撑到 v1.0 的骨架**，同时保持"为学习服务"——每一层都必须是
理解网络编程所必需的，不能是为了好看而加的抽象。

## 决策

按职责分四层：

```
chat      业务：User / Room / ChatHub / Command     不认识 socket
protocol  Wire 格式：Frame / FrameCodec             纯函数，不认识 I/O
net       连接与事件：Connection / Buffer / EventLoop  只认识字节流
platform  系统调用封装：UniqueFd / Address / errno
```

**依赖只能向下、且只能相邻：`chat → protocol → net → platform`。**

### 为什么用链接关系强制而不是文档规定

CMake 的 target 划分让"违反分层"变成**链接错误**：

```cmake
target_link_libraries(chatroom_net PUBLIC chatroom_platform)   # 拿不到 chat
```

`net` 想 include `chat/` 的头文件，编译能过（头文件路径都在一起），但一旦
引用 `chat` 里的符号就链接失败。这比写在文档里"请不要反向依赖"可靠得多。

## 备选方案

| 方案 | 否决理由 |
|---|---|
| 保持单文件，只做功能 | 三周后自己都读不懂；无法测试；简历上毫无说服力 |
| 用 muduo / asio | 学习目标就是 epoll 和缓冲区，引框架等于放弃目标 |
| 按 MVC 分（model/view/controller） | 网络服务没有"视图"，硬套只会产生空层 |
| 按功能分（join/nick/msg 各一模块） | 每加一个命令就要新建模块，且共享状态无处安放 |

muduo 是优秀的**参考读物**（陈硕的书），但只读不引。

## 一个实现期的修正

初版设计写的是 `chat → protocol → net`，落地时发现 `chat` 根本不需要 `net`：

- 业务层产出 `Outbound{ConnectionId, bytes}`
- `ConnectionId` 是 `uint64_t` 别名，不是 socket
- 真正把 `ConnectionId` 映射到 `Connection` 的，是最上层的装配代码

因此 `chat` 只依赖 `protocol`。直接收益：**macOS 上也能编译并测试业务层**，
CI 的 macOS job 因此可以在 M2（引入 epoll）之后继续为绿色。这是在给 CI
配矩阵时发现的，记在此处作为设计被验证的证据。

## 后果

**收益**

- 业务逻辑可以脱离网络单测（喂 `Frame`，断言 `Outbound`）
- 协议层可以穷举测试（半包、粘包、超长帧）
- 换网络模型（M6 线程池、M8 WebSocket）不动业务层

**代价**

- 头文件数量和 target 数量增加，小项目会显得"重"
- 跨层传递需要额外的值类型（`ConnectionId`、`Outbound`）
- 新人需要先理解依赖方向才能动手

对教学项目而言，代价可以接受：这些正是要学的东西。
