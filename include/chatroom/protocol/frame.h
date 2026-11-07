// frame.h — 一条应用层消息的内存表示
//
// 所属层：protocol
// 被谁使用：frame_codec（编解码）、chat 层（业务处理）
//
// 设计取舍：payload 用 vector<string> 而不是每种消息一个结构体。
//
//   代价：编译期拿不到类型检查，"MSG 的第 0 个字段是房间名"这种约定
//         只能靠文档和测试保证。
//   收益：加一个新命令不用改协议层，只要加一个 type 值。
//
// 对一个小型聊天协议，这个交换是划算的；对大型协议（几百种消息）就该
// 换成专门的序列化框架了。

#pragma once

#include "chatroom/protocol/message_type.h"

#include <string>
#include <vector>

namespace chatroom::protocol {

/// 一帧的逻辑内容。字段的语义由 type 决定，见 message_type.h 的注释。
struct Frame {
    MessageType type = MessageType::kSys;
    std::vector<std::string> fields;
};

}  // namespace chatroom::protocol
