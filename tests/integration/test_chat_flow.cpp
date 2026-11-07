#include "chatroom/protocol/frame.h"

#include "helpers/test_client.h"
#include "helpers/test_server.h"

#include <gtest/gtest.h>

using chatroom::protocol::Frame;
using chatroom::protocol::MessageType;
using chatroom::testing::TestClient;
using chatroom::testing::TestServer;

namespace {

Frame frame_of(MessageType type, std::vector<std::string> fields) {
    Frame f;
    f.type = type;
    f.fields = std::move(fields);
    return f;
}

// 发送 JOIN 并等到服务器确认。
//
// 这一步不能省：两个客户端的 JOIN 走的是不同 socket，服务端处理顺序
// 不保证等于发送顺序。如果 A 在 B 的 JOIN 被处理之前就发消息，那条消息
// 自然只有 A 自己在房间里能收到——测试会以"偶发失败"的形式惩罚这种写法。
bool join_and_wait(TestClient& client, const std::string& nickname) {
    if (!client.send_frame(frame_of(MessageType::kJoin, {nickname}))) {
        return false;
    }
    for (int i = 0; i < 10; ++i) {
        Frame f;
        if (!client.recv_frame(f)) {
            return false;
        }
        if (f.type == MessageType::kSys && f.fields.size() == 2 &&
            f.fields[1].find(nickname) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// 收消息直到收到指定类型（或超时）。房间里夹杂着 join/leave 通知，
// 直接断言"下一条就是 msg"会很脆。
bool recv_until(TestClient& client, MessageType type, Frame& out, int max_frames = 10) {
    for (int i = 0; i < max_frames; ++i) {
        Frame f;
        if (!client.recv_frame(f)) {
            return false;
        }
        if (f.type == type) {
            out = std::move(f);
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(ChatFlow, TwoClientsExchangeMessages) {
    TestServer server(/*worker_threads=*/0);
    ASSERT_TRUE(server.valid());

    TestClient alice;
    TestClient bob;
    ASSERT_TRUE(alice.connect_to(server.port()));
    ASSERT_TRUE(bob.connect_to(server.port()));

    ASSERT_TRUE(join_and_wait(alice, "alice"));
    ASSERT_TRUE(join_and_wait(bob, "bob"));

    ASSERT_TRUE(alice.send_frame(frame_of(MessageType::kMsg, {"", "hello bob"})));

    Frame received;
    ASSERT_TRUE(recv_until(bob, MessageType::kMsg, received)) << "bob did not receive the message";
    EXPECT_EQ(received.fields[0], "alice");
    EXPECT_EQ(received.fields[2], "hello bob");

    // 发送者自己也应该收到（客户端据此显示自己发的内容）
    Frame echo;
    ASSERT_TRUE(recv_until(alice, MessageType::kMsg, echo));
    EXPECT_EQ(echo.fields[2], "hello bob");
}

TEST(ChatFlow, DirectMessageIsNotBroadcast) {
    TestServer server(0);
    ASSERT_TRUE(server.valid());

    TestClient alice;
    TestClient bob;
    TestClient carol;
    ASSERT_TRUE(alice.connect_to(server.port()));
    ASSERT_TRUE(bob.connect_to(server.port()));
    ASSERT_TRUE(carol.connect_to(server.port()));

    ASSERT_TRUE(join_and_wait(alice, "alice"));
    ASSERT_TRUE(join_and_wait(bob, "bob"));
    ASSERT_TRUE(join_and_wait(carol, "carol"));

    ASSERT_TRUE(alice.send_frame(frame_of(MessageType::kDm, {"bob", "psst"})));

    Frame received;
    ASSERT_TRUE(recv_until(bob, MessageType::kDm, received));
    EXPECT_EQ(received.fields[0], "alice");
    EXPECT_EQ(received.fields[1], "psst");

    // carol 不该收到私聊：把她收到的消息全读出来，确认没有 kDm
    Frame junk;
    while (carol.recv_frame(junk, 200)) {
        EXPECT_NE(junk.type, MessageType::kDm);
    }
}

TEST(ChatFlow, WorksWithWorkerThreads) {
    // 和第一个用例同样的流程，但业务跑在线程池里。
    // 这条用例的价值在 TSan 构建下：能抓出跨线程的数据竞争。
    TestServer server(/*worker_threads=*/2);
    ASSERT_TRUE(server.valid());

    TestClient alice;
    TestClient bob;
    ASSERT_TRUE(alice.connect_to(server.port()));
    ASSERT_TRUE(bob.connect_to(server.port()));

    ASSERT_TRUE(join_and_wait(alice, "alice"));
    ASSERT_TRUE(join_and_wait(bob, "bob"));

    ASSERT_TRUE(alice.send_frame(frame_of(MessageType::kMsg, {"", "threaded hello"})));

    Frame received;
    ASSERT_TRUE(recv_until(bob, MessageType::kMsg, received, 20));
    EXPECT_EQ(received.fields[2], "threaded hello");
}

TEST(ChatFlow, ProtocolErrorClosesConnection) {
    TestServer server(0);
    ASSERT_TRUE(server.valid());

    TestClient client;
    ASSERT_TRUE(client.connect_to(server.port()));

    // 4 字节长度 = 1 MiB，远超 64 KiB 上限。服务端必须判定协议错误并断开，
    // 而不是傻等 1 MiB 数据。
    const char junk[] = {'\x10', '\x00', '\x00', '\x00'};
    ASSERT_TRUE(client.send_raw(junk, sizeof(junk)));

    EXPECT_TRUE(client.wait_for_eof(2000));
}
