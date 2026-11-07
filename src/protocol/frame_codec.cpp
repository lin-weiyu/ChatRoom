#include "chatroom/protocol/frame_codec.h"

#include <arpa/inet.h>

#include <cstring>
#include <string>

namespace chatroom::protocol {

namespace {

// 从 p 处读一个 16 位大端整数
std::uint16_t read_u16(const char* p) {
    std::uint16_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return ntohs(v);
}

void append_u16(std::string& out, std::uint16_t v) {
    const std::uint16_t be = htons(v);
    out.append(reinterpret_cast<const char*>(&be), sizeof(be));
}

}  // namespace

std::string encode(const Frame& frame) {
    std::string payload;

    for (const std::string& field : frame.fields) {
        if (field.size() > kMaxFieldSize) {
            return {};  // 编程错误：字段超长
        }
        append_u16(payload, static_cast<std::uint16_t>(field.size()));
        payload.append(field);
    }

    // len 覆盖 type + payload，所以是 1 + payload.size()
    const std::uint32_t len = static_cast<std::uint32_t>(1 + payload.size());
    if (len > kMaxFrameSize) {
        return {};
    }

    std::string out;
    out.reserve(kHeaderSize + payload.size());

    const std::uint32_t be_len = htonl(len);
    out.append(reinterpret_cast<const char*>(&be_len), sizeof(be_len));
    out.push_back(static_cast<char>(frame.type));
    out.append(payload);

    return out;
}

DecodeResult try_decode(std::string_view data) {
    DecodeResult result;

    if (data.size() < sizeof(std::uint32_t)) {
        result.status = DecodeStatus::kIncomplete;
        return result;
    }

    std::uint32_t len = 0;
    std::memcpy(&len, data.data(), sizeof(len));
    len = ntohl(len);

    // 长度必须在 [1, kMaxFrameSize] 之间：
    //   0   —— 连 type 都没有，非法
    //   过大 —— 要么是攻击，要么是字节流已经错位
    if (len < 1 || len > kMaxFrameSize) {
        result.status = DecodeStatus::kError;
        result.error = "frame length out of range: " + std::to_string(len);
        return result;
    }

    if (data.size() < sizeof(std::uint32_t) + len) {
        result.status = DecodeStatus::kIncomplete;
        return result;
    }

    result.frame.type = static_cast<MessageType>(data[sizeof(std::uint32_t)]);

    std::string_view payload = data.substr(kHeaderSize, len - 1);

    while (!payload.empty()) {
        if (payload.size() < sizeof(std::uint16_t)) {
            result.status = DecodeStatus::kError;
            result.error = "truncated field length";
            return result;
        }

        const std::uint16_t field_len = read_u16(payload.data());
        payload.remove_prefix(sizeof(std::uint16_t));

        if (payload.size() < field_len) {
            result.status = DecodeStatus::kError;
            result.error = "field length exceeds payload";
            return result;
        }

        result.frame.fields.emplace_back(payload.substr(0, field_len));
        payload.remove_prefix(field_len);
    }

    result.status = DecodeStatus::kOk;
    // len 已经包含了 type 那 1 字节，所以总占用是 4 + len，不是 5 + len。
    result.consumed = sizeof(std::uint32_t) + len;
    return result;
}

}  // namespace chatroom::protocol
