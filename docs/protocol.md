# 应用层协议（Wire Protocol）v1

> 本文是协议的**唯一真相**。代码与本文不一致时，以本文为准并修代码。

## 1. 帧格式

```
+------------------+--------+---------------------+
|    length (4B)   | type   |      payload        |
|   network order  | (1B)   |  length - 1 bytes   |
+------------------+--------+---------------------+
```

| 字段 | 大小 | 说明 |
|---|---|---|
| `length` | 4 字节，大端 | `1 + payload.size()`，即"type + payload"的总长度 |
| `type` | 1 字节 | 见消息类型表 |
| `payload` | `length - 1` 字节 | 字段序列，见下 |

**为什么是长度前缀，不是换行符分隔：**

TCP 是字节流，不保留消息边界。用 `\n` 分隔意味着每次都要扫描、转义 payload 中的
换行，且收到半个字符序列时无法判断。长度前缀把"找边界"变成"读 4 字节头"，
O(1) 且永不误判。

`length` 上限 64 KiB。超出直接判定为协议错误并断开——否则 4 个恶意字节就能让服务
分配几百 MB 内存。

## 2. 字段编码

payload 是一串字段，每个字段：

```
+----------------+------------------+
| len (2B, 大端) | utf-8 bytes      |
+----------------+------------------+
```

字段按顺序解析，直到 payload 耗尽。字符串可以包含任意字节（包括 `\0`），因为长度
由前缀显式给出，不依赖终止符。

## 3. 消息类型

| 值 | 名称 | 方向 | 字段 |
|---|---|---|---|
| `0x01` | JOIN | C→S | `nickname` |
| `0x02` | NICK | C→S | `new_nickname` |
| `0x03` | MSG | C→S | `room`（空 = 当前房间）, `text` |
| `0x04` | DM | C→S | `target_nickname`, `text` |
| `0x05` | QUIT | C→S | — |
| `0x06` | SYS | S→C | `code`, `text` |
| `0x07` | PING | C→S | — |
| `0x08` | PONG | C→S | — |
| `0x09` | JOIN_ROOM | C→S | `room` |
| `0x0A` | LEAVE_ROOM | C→S | `room` |
| `0x0B` | LIST | C→S | — |

`SYS.code` 取值：`info`、`error`、`join`、`leave`。

## 4. 会话时序

```
C: JOIN  {nickname="alice"}
S: SYS   {code="info", text="welcome, alice"}
S: SYS   {code="join", text="alice joined lobby"}   ← 广播给其他人

C: MSG   {room="", text="hello"}
S: SYS   {code="info"} …                            ← 见下
S: … 广播给同房间所有人（含发送者）

C: QUIT  {}
S: 关闭连接
```

## 5. 错误处理

| 情况 | 处理 |
|---|---|
| `length` 为 0 或 > 64 KiB | 断开连接 |
| 字段长度前缀越界 | 断开连接 |
| 未知 `type` | 忽略该帧，回 `SYS{code="error"}` |
| 未 JOIN 就发 MSG | 回 `SYS{code="error"}`，连接保留 |
| 昵称重复 | 回 `SYS{code="error"}`，保留原名 |
| 发送到未加入的房间 | 回 `SYS{code="error"}` |

## 6. 演进规则

- **只追加**：新类型往后加，旧类型永不删除或改语义
- 字段只追加在末尾，已有字段的位置和含义不变
- 客户端必须忽略不认识的 SYS code
- 破坏性变更 = 新协议版本（v2 走一个新的 type 范围，而不是改旧 type）

## 7. 参考实现

- 编解码：`src/protocol/frame_codec.cpp`
- 测试：`tests/unit/test_frame_codec.cpp`（半包、粘包、超长、截断字段）
- 客户端：`tools/chat_cli.cpp`
