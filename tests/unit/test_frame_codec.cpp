#include "chatroom/protocol/frame_codec.h"

#include <arpa/inet.h>
#include <string>

#include <gtest/gtest.h>

using chatroom::protocol::DecodeStatus;
using chatroom::protocol::Frame;
using chatroom::protocol::MessageType;
using chatroom::protocol::try_decode;

namespace {

Frame make_frame(MessageType type, std::vector<std::string> fields) {
    Frame f;
    f.type = type;
    f.fields = std::move(fields);
    return f;
}

}  // namespace

TEST(FrameCodec, RoundTripEmptyFrame) {
    const Frame original = make_frame(MessageType::kQuit, {});
    const std::string bytes = chatroom::protocol::encode(original);
    ASSERT_FALSE(bytes.empty());

    const auto r = try_decode(bytes);
    ASSERT_EQ(r.status, DecodeStatus::kOk);
    EXPECT_EQ(r.frame.type, MessageType::kQuit);
    EXPECT_TRUE(r.frame.fields.empty());
    EXPECT_EQ(r.consumed, bytes.size());
}

TEST(FrameCodec, RoundTripMultipleFields) {
    const Frame original = make_frame(MessageType::kMsg, {"lobby", "hello world"});
    const std::string bytes = chatroom::protocol::encode(original);

    const auto r = try_decode(bytes);
    ASSERT_EQ(r.status, DecodeStatus::kOk);
    ASSERT_EQ(r.frame.fields.size(), 2u);
    EXPECT_EQ(r.frame.fields[0], "lobby");
    EXPECT_EQ(r.frame.fields[1], "hello world");
}

TEST(FrameCodec, HandlesUtf8AndBinary) {
    const std::string text = "你好，世界";
    const std::string binary = std::string("a\0b", 3);

    const Frame original = make_frame(MessageType::kMsg, {text, binary});
    const auto r = try_decode(chatroom::protocol::encode(original));

    ASSERT_EQ(r.status, DecodeStatus::kOk);
    EXPECT_EQ(r.frame.fields[0], text);
    EXPECT_EQ(r.frame.fields[1], binary);
}

TEST(FrameCodec, IncompleteHeaderWaits) {
    // 只有 2 字节，连长度都没读全
    const auto r = try_decode("\x00\x00");
    EXPECT_EQ(r.status, DecodeStatus::kIncomplete);
}

TEST(FrameCodec, IncompletePayloadWaits) {
    const std::string full = chatroom::protocol::encode(make_frame(MessageType::kMsg, {"hi"}));

    for (std::size_t cut = 4; cut < full.size(); ++cut) {
        const auto r = try_decode(std::string_view(full).substr(0, cut));
        EXPECT_EQ(r.status, DecodeStatus::kIncomplete) << "cut at " << cut;
    }
}

TEST(FrameCodec, DecodesFirstFrameAndReportsConsumed) {
    // 两个帧粘在一起：解码器必须只吃第一个，并告诉调用方吃了多少
    const std::string a = chatroom::protocol::encode(make_frame(MessageType::kPing, {}));
    const std::string b = chatroom::protocol::encode(make_frame(MessageType::kMsg, {"x"}));
    const std::string glued = a + b;

    const auto first = try_decode(glued);
    ASSERT_EQ(first.status, DecodeStatus::kOk);
    EXPECT_EQ(first.frame.type, MessageType::kPing);
    EXPECT_EQ(first.consumed, a.size());

    const auto second = try_decode(std::string_view(glued).substr(first.consumed));
    ASSERT_EQ(second.status, DecodeStatus::kOk);

    // 消费完两个帧后，剩余字节应为 0——这是"粘包不丢数据"的直接证明
    EXPECT_EQ(first.consumed + second.consumed, glued.size());
}

TEST(FrameCodec, RejectsZeroLength) {
    // len = 0：连 type 都没有，属于非法帧
    const std::string bad("\x00\x00\x00\x00", 4);
    const auto r = try_decode(bad);
    EXPECT_EQ(r.status, DecodeStatus::kError);
}

TEST(FrameCodec, RejectsOversizedLength) {
    // len = 1 MiB，远超 64 KiB 上限。攻击者用这 4 个字节就能让朴素实现
    // 去分配 1 MiB 内存并无限等待。
    const std::string bad("\x00\x10\x00\x00", 4);
    const auto r = try_decode(bad);
    EXPECT_EQ(r.status, DecodeStatus::kError);
}

TEST(FrameCodec, RejectsTruncatedFieldLength) {
    // 头部说 len=2（type + 1 字节），但那 1 字节不足以构成一个字段长度
    std::string bad;
    const std::uint32_t len = htonl(2);
    bad.append(reinterpret_cast<const char*>(&len), 4);
    bad.push_back(static_cast<char>(MessageType::kMsg));
    bad.push_back('\x00');

    const auto r = try_decode(bad);
    EXPECT_EQ(r.status, DecodeStatus::kError);
}

TEST(FrameCodec, RejectsFieldLongerThanPayload) {
    // 字段声明 10 字节，但 payload 只剩 3 字节
    std::string payload;
    const std::uint16_t field_len = htons(10);
    payload.append(reinterpret_cast<const char*>(&field_len), 2);
    payload.append("abc");

    std::string bad;
    const std::uint32_t len = htonl(static_cast<std::uint32_t>(1 + payload.size()));
    bad.append(reinterpret_cast<const char*>(&len), 4);
    bad.push_back(static_cast<char>(MessageType::kMsg));
    bad.append(payload);

    const auto r = try_decode(bad);
    EXPECT_EQ(r.status, DecodeStatus::kError);
}

TEST(FrameCodec, EncodeRejectsOversizedField) {
    const std::string huge(chatroom::protocol::kMaxFieldSize + 1, 'x');
    EXPECT_TRUE(chatroom::protocol::encode(make_frame(MessageType::kMsg, {huge})).empty());
}
