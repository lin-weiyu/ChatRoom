#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "chatroom/protocol/frame.h"

namespace chatroom::chat {

// 会话标识。
//
// 注意它不是 fd：fd 会被复用（关闭后同一号码可能被新连接占用），
// 用它做业务主键会出现"消息发给了错误的用户"这类事故。
// 会话号单调递增，永不复用。
using SessionId = std::uint64_t;

inline constexpr SessionId kInvalidSession = 0;

// 业务层产生的"出站意图"。
//
// 关键设计：ChatHub 不知道 socket 长什么样，它只说"发给谁、发什么"。
// 由装配层（app::ChatServer）翻译成 conn->send()。这样 ChatHub 可以完全
// 脱离网络单测。
struct Outbound {
    SessionId to = kInvalidSession;
    protocol::Frame frame;
    bool close_after_send = false;
};

struct UserInfo {
    SessionId id = kInvalidSession;
    std::string nickname;
    std::string room;
    std::string peer;
};

}  // namespace chatroom::chat
