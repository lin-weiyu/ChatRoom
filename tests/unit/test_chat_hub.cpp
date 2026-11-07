#include "chatroom/chat/chat_hub.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using chatroom::chat::ChatHub;
using chatroom::chat::Outbound;
using chatroom::chat::SessionId;
using chatroom::protocol::Frame;
using chatroom::protocol::MessageType;

namespace {

SessionId join(ChatHub& hub, const std::string& nick, std::vector<Outbound>& out) {
    std::vector<Outbound> welcome;
    const SessionId id = hub.add_session("127.0.0.1:1000", welcome);

    Frame f;
    f.type = MessageType::kJoin;
    f.fields = {nick};
    hub.handle(id, f, out);
    return id;
}

std::vector<Outbound> to(const std::vector<Outbound>& out, SessionId id) {
    std::vector<Outbound> result;
    for (const auto& o : out) {
        if (o.to == id) {
            result.push_back(o);
        }
    }
    return result;
}

}  // namespace

TEST(ChatHub, NewSessionGetsWelcome) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId id = hub.add_session("peer", out);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].to, id);
    EXPECT_EQ(out[0].frame.type, MessageType::kSys);
    EXPECT_EQ(out[0].frame.fields[0], "info");
}

TEST(ChatHub, JoinSetsNickname) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId id = join(hub, "alice", out);

    ASSERT_NE(hub.find_user(id), nullptr);
    EXPECT_EQ(hub.find_user(id)->nickname, "alice");
    EXPECT_EQ(hub.find_user(id)->room, ChatHub::kDefaultRoom);
}

TEST(ChatHub, DuplicateNicknameRejected) {
    ChatHub hub;
    std::vector<Outbound> out;
    join(hub, "alice", out);

    std::vector<Outbound> out2;
    join(hub, "alice", out2);

    ASSERT_EQ(out2.size(), 1u);
    EXPECT_EQ(out2[0].frame.fields[0], "error");
}

TEST(ChatHub, MessageIsBroadcastToWholeRoom) {
    ChatHub hub;
    std::vector<Outbound> a_out;
    std::vector<Outbound> b_out;
    const SessionId a = join(hub, "alice", a_out);
    const SessionId b = join(hub, "bob", b_out);

    std::vector<Outbound> out;
    Frame msg;
    msg.type = MessageType::kMsg;
    msg.fields = {"", "hello"};
    hub.handle(a, msg, out);

    // 两个人都应该收到，发送者的昵称由服务端补进帧里
    ASSERT_EQ(out.size(), 2u);
    for (const auto& o : out) {
        EXPECT_EQ(o.frame.type, MessageType::kMsg);
        EXPECT_EQ(o.frame.fields[0], "alice");
        EXPECT_EQ(o.frame.fields[2], "hello");
    }
    EXPECT_TRUE(to(out, a).size() == 1u || to(out, b).size() == 1u);
}

TEST(ChatHub, MessageBeforeJoinIsRejected) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId id = hub.add_session("peer", out);

    std::vector<Outbound> result;
    Frame msg;
    msg.type = MessageType::kMsg;
    msg.fields = {"", "hello"};
    hub.handle(id, msg, result);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].frame.fields[0], "error");
}

TEST(ChatHub, DirectMessageReachesOnlyTarget) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);
    const SessionId b = join(hub, "bob", out);

    std::vector<Outbound> result;
    Frame dm;
    dm.type = MessageType::kDm;
    dm.fields = {"bob", "secret"};
    hub.handle(a, dm, result);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].to, b);
    EXPECT_EQ(result[0].frame.fields[0], "alice");
    EXPECT_EQ(result[0].frame.fields[1], "secret");
}

TEST(ChatHub, DirectMessageToUnknownUserIsRejected) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);

    std::vector<Outbound> result;
    Frame dm;
    dm.type = MessageType::kDm;
    dm.fields = {"nobody", "hi"};
    hub.handle(a, dm, result);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].to, a);
    EXPECT_EQ(result[0].frame.fields[0], "error");
}

TEST(ChatHub, LeavingNotifiesRoom) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);
    join(hub, "bob", out);

    std::vector<Outbound> result;
    hub.remove_session(a, result);

    EXPECT_EQ(hub.session_count(), 1u);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result[0].frame.fields[0], "leave");
}

TEST(ChatHub, JoinRoomMovesUser) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);

    std::vector<Outbound> result;
    Frame f;
    f.type = MessageType::kJoinRoom;
    f.fields = {"dev"};
    hub.handle(a, f, result);

    EXPECT_EQ(hub.find_user(a)->room, "dev");
    EXPECT_EQ(hub.room_count(), 2u);
}

TEST(ChatHub, PingIsAnswered) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);

    std::vector<Outbound> result;
    Frame f;
    f.type = MessageType::kPing;
    hub.handle(a, f, result);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].frame.fields[0], "pong");
}

TEST(ChatHub, QuitRequestsClose) {
    ChatHub hub;
    std::vector<Outbound> out;
    const SessionId a = join(hub, "alice", out);

    std::vector<Outbound> result;
    Frame f;
    f.type = MessageType::kQuit;
    hub.handle(a, f, result);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].close_after_send);
}
