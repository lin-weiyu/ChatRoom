#include <gtest/gtest.h>

#include "chatroom/version.h"

// M0 的烟雾测试：证明构建、链接、测试框架三段链路都通。
// 真正的协议/业务测试从 M1 开始。
TEST(Version, IsNotEmpty) { EXPECT_FALSE(chatroom::version().empty()); }

TEST(Version, BuildTypeIsKnown) {
    const auto type = chatroom::build_type();
    EXPECT_TRUE(type == "Debug" || type == "Release");
}
