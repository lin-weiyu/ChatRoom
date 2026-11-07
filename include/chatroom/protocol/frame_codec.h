#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "chatroom/protocol/frame.h"

namespace chatroom::protocol {

// 单帧总长上限（含 5 字节头）。
// 不设上限的话，一个恶意的 4 字节长度就能让服务端无限等数据或者分配
// 数百 MB 内存。
inline constexpr std::size_t kMaxFrameSize = 64 * 1024;

// 单个字段上限，受 u16 长度前缀限制
inline constexpr std::size_t kMaxFieldSize = 64 * 1024 - 1;

inline constexpr std::size_t kHeaderSize = 5;

enum class DecodeStatus {
    kOk,          // 取出一帧，consumed 是它占用的字节数
    kIncomplete,  // 数据不够，等下一次 recv
    kError,       // 字节流已经不可信，必须断开连接
};

struct DecodeResult {
    DecodeStatus status = DecodeStatus::kIncomplete;
    std::size_t consumed = 0;
    Frame frame;
    std::string error;
};

// 把一帧编码成字节串。字段超长时返回空串（调用方应把它当成编程错误）。
std::string encode(const Frame& frame);

// 从字节串前端尝试取一帧。
//
// 这是纯函数：不碰 socket、不碰缓冲区、不修改入参。正因为如此，
// 它可以被穷举测试——半包、粘包、超长帧、字段长度溢出都在这里被挡住，
// 而不是在上线之后。
DecodeResult try_decode(std::string_view data);

}  // namespace chatroom::protocol
