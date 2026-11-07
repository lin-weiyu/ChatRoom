#include "chatroom/net/event_fd.h"

#include <gtest/gtest.h>

using chatroom::net::EventFd;

// 这组测试只在 Linux 上跑：eventfd 是 Linux 特有的系统调用。
TEST(EventFd, NotifyAndDrain) {
    EventFd efd;
    ASSERT_TRUE(efd.valid());

    efd.notify();
    efd.drain();  // 不应阻塞

    efd.notify();
    efd.notify();
    efd.drain();  // 累计计数一次读走
}
