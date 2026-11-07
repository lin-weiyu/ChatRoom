#pragma once

#include <cstdint>

namespace chatroom::protocol {

// 消息类型。取值一旦发布就不允许改变——它出现在线上字节流里。
//
// 新增类型只能往后追加；删除类型会让旧客户端把未知字节当成错误。
enum class MessageType : std::uint8_t {
    kJoin = 0x01,       // C->S  {nickname}
    kNick = 0x02,       // C->S  {new_nickname}
    kMsg = 0x03,        // C->S  {room, text}
    kDm = 0x04,         // C->S  {target_nickname, text}
    kQuit = 0x05,       // C->S  {}
    kSys = 0x06,        // S->C  {code, text}
    kPing = 0x07,       // C->S  {}  心跳请求
    kPong = 0x08,       // C->S  {}  心跳应答
    kJoinRoom = 0x09,   // C->S  {room}
    kLeaveRoom = 0x0A,  // C->S  {room}
    kList = 0x0B,       // C->S  {}  列出房间
};

}  // namespace chatroom::protocol
