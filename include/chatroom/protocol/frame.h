#pragma once

#include "chatroom/protocol/message_type.h"

#include <string>
#include <vector>

namespace chatroom::protocol {

// 一条应用层消息。
//
// payload 被编码成一串"长度前缀字符串"（见 docs/protocol.md）。
// 这里用 vector<string> 而不是每种消息一个结构体，是为了让协议层保持
// 纯粹：它只负责切分字段，字段的语义由 chat 层按 type 解释。
//
// 代价是编译期拿不到类型检查；收益是加一个新命令不用改协议层。
struct Frame {
    MessageType type = MessageType::kSys;
    std::vector<std::string> fields;
};

}  // namespace chatroom::protocol
