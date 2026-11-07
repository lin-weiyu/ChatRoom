// frame_codec.h — 帧的编解码（纯函数）
//
// 所属层：protocol
// 被谁使用：app::ChatServer（解析收到的字节、编码要发出的帧）、tools/chat_cli
//
// 为什么强调"纯函数"：不碰 socket、不碰缓冲区、不修改任何全局状态。
// 正因如此，它可以被穷举测试——半包、粘包、超长帧、截断字段都在这里
// 被挡住，而不是在上线之后。

#pragma once

#include "chatroom/protocol/frame.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace chatroom::protocol {

/// 单帧总长上限（含 5 字节头）。
///
/// 不设上限的话，一个恶意的 4 字节长度就能让服务端无限等数据，
/// 或者让朴素实现去分配几百 MB 内存。
inline constexpr std::size_t kMaxFrameSize = 64 * 1024;

/// 单个字段上限，受 u16 长度前缀限制。
inline constexpr std::size_t kMaxFieldSize = 64 * 1024 - 1;

/// 帧头长度：4 字节长度 + 1 字节类型。
inline constexpr std::size_t kHeaderSize = 5;

/// 一次解码尝试的结果。
enum class DecodeStatus {
    kOk,          ///< 取出一帧，consumed 是它占用的字节数
    kIncomplete,  ///< 数据还不够，等下一次 recv（不是错误）
    kError,       ///< 字节流已经不可信，必须断开连接
};

struct DecodeResult {
    DecodeStatus status = DecodeStatus::kIncomplete;

    /// 本次消费掉的字节数，仅在 status == kOk 时有效。
    /// 调用方应当 buffer.retrieve(consumed) 之后再尝试解下一帧。
    std::size_t consumed = 0;

    Frame frame;
    std::string error;  ///< status == kError 时的可读原因，用于日志
};

/// 把一帧编码成字节串。
///
/// @return 编码结果；字段超长或整帧超限时返回空串（调用方应视为编程错误）
std::string encode(const Frame& frame);

/// 从字节串前端尝试取一帧。
///
/// @param data 可以是任意长度：可能连帧头都不够，也可能包含好几帧
/// @return     三种状态之一，见 DecodeStatus
///
/// 使用方式（这是粘包/半包处理的核心循环）：
///
///     for (;;) {
///         auto r = try_decode(buffer.view());
///         if (r.status == kIncomplete) break;   // 等更多数据
///         if (r.status == kError) { close(); }  // 字节流错位
///         buffer.retrieve(r.consumed);
///         handle(r.frame);
///     }
DecodeResult try_decode(std::string_view data);

}  // namespace chatroom::protocol
