// message_type.h — 消息类型枚举
//
// 所属层：protocol
// 被谁使用：frame.h、chat 层（按类型分发业务逻辑）
//
// 这些数字会直接出现在网线上，一旦发布就不允许改变语义。
// 新增类型只能往后追加；删除或改语义会让旧客户端解析出错。

#pragma once

#include <cstdint>

namespace chatroom::protocol {

/// 帧头里的 type 字段（1 字节）。
///
/// 方向说明：C→S 表示客户端发给服务端，S→C 反之。
enum class MessageType : std::uint8_t {
    kJoin = 0x01,  ///< C→S {nickname}            加入聊天室
    kNick = 0x02,  ///< C→S {new_nickname}        改名
    kMsg = 0x03,   ///< C→S {room, text}          发消息（room 为空表示当前房间）
                   ///< S→C {nickname, room, text} 服务端补上发送者昵称后广播
    kDm = 0x04,    ///< C→S {target_nickname, text} 私聊
                   ///< S→C {from_nickname, text, "dm"}
    kQuit = 0x05,  ///< C→S {}                    主动退出
    kSys = 0x06,  ///< S→C {code, text}          服务端通知（code: info/error/join/leave/pong）
    kPing = 0x07,       ///< C→S {}                    心跳请求
    kPong = 0x08,       ///< C→S {}                    心跳应答
    kJoinRoom = 0x09,   ///< C→S {room}                进入指定房间
    kLeaveRoom = 0x0A,  ///< C→S {room}                离开房间，回到大厅
    kList = 0x0B,       ///< C→S {}                    列出房间
};

}  // namespace chatroom::protocol
