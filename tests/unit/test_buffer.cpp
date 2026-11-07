#include <gtest/gtest.h>

#include <string>

#include "chatroom/net/buffer.h"

using chatroom::net::Buffer;

TEST(Buffer, StartsEmpty) {
    Buffer b;
    EXPECT_EQ(b.readable(), 0u);
    EXPECT_TRUE(b.view().empty());
}

TEST(Buffer, AppendAndRetrieve) {
    Buffer b;
    b.append("hello");
    EXPECT_EQ(b.readable(), 5u);
    EXPECT_EQ(b.view(), "hello");

    b.retrieve(2);
    EXPECT_EQ(b.view(), "llo");
}

TEST(Buffer, RetrieveMoreThanAvailableIsClamped) {
    // 防御性设计：上层算出错误的长度不该导致越界。
    Buffer b;
    b.append("abc");
    b.retrieve(100);
    EXPECT_EQ(b.readable(), 0u);
}

TEST(Buffer, SurvivesManyAppends) {
    // 反复读写，检查扩容与压缩后数据仍然正确。
    Buffer b;
    std::string expected;
    for (int i = 0; i < 1000; ++i) {
        const std::string piece = "chunk" + std::to_string(i) + ";";
        b.append(piece);
        expected += piece;

        // 每 3 次消费一次，制造"头部有空洞"的局面
        if (i % 3 == 0 && b.readable() > 7) {
            b.retrieve(7);
            expected.erase(0, 7);
        }
    }
    EXPECT_EQ(b.view(), expected);
}

TEST(Buffer, HandlesBinaryDataWithNullBytes) {
    // TCP 是字节流，'\0' 就是普通字节。用 std::string 的 c_str 语义
    // 处理网络数据是经典错误，这里用 view + length 保证安全。
    const char raw[] = {'a', '\0', 'b', '\0'};
    Buffer b;
    b.append(raw, sizeof(raw));
    EXPECT_EQ(b.readable(), 4u);
    EXPECT_EQ(b.view().size(), 4u);
    EXPECT_EQ(b.view()[1], '\0');
}
