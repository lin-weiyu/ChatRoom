// types.h — 业务层的核心值类型
//
// 所属层：chat
// 被谁使用：chat 层全部、app 层（把 Outbound 翻译成 send）
//
// 这个文件很小，但定义了整个业务层与网络层之间的"接口协议"。

#pragma once

#include "chatroom/protocol/frame.h"

#include <cstdint>
#include <string>
#include <vector>

namespace chatroom::chat {

/// 会话标识。单调递增，永不复用。
///
/// 为什么不能用 fd 当主键：fd 会被内核回收复用。用户 A 断开后，同一个
/// fd 号可能马上分配给用户 B；如果业务表里还留着旧映射，消息就会发给
/// 完全不相干的人。这类 bug 极难复现，因为依赖"刚好复用"的时序。
using SessionId = std::uint64_t;

/// 无效会话，用于"没有接收者"的场合。
inline constexpr SessionId kInvalidSession = 0;

/// 业务层产生的"出站意图"。
///
/// 关键设计：ChatHub 不知道 socket 长什么样，它只说"发给谁、发什么"。
/// 由装配层（app::ChatServer）翻译成 conn->send()。
///
/// 这样带来的直接收益：ChatHub 可以完全脱离网络做单元测试——
/// 喂一个 Frame，断言返回的 Outbound 内容即可。
struct Outbound {
    SessionId to = kInvalidSession;  ///< 目标会话
    protocol::Frame frame;           ///< 要发送的内容
    bool close_after_send = false;   ///< 发完是否关闭连接（QUIT 用）
};

/// 一个用户的业务状态。不含 fd——那是网络层的事。
struct UserInfo {
    SessionId id = kInvalidSession;
    std::string nickname;  ///< 为空表示还没 JOIN
    std::string room;      ///< 当前所在房间
    std::string peer;      ///< "ip:port"，仅用于日志
};

}  // namespace chatroom::chat
