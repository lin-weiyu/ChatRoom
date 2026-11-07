#include "chatroom/platform/unique_fd.h"

#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>

using chatroom::platform::UniqueFd;

namespace {
int make_pipe_fd(int fds[2]) {
    if (::pipe(fds) != 0) {
        return -1;
    }
    return 0;
}
}  // namespace

TEST(UniqueFd, DefaultIsInvalid) {
    UniqueFd fd;
    EXPECT_FALSE(fd.valid());
    EXPECT_EQ(fd.get(), -1);
}

TEST(UniqueFd, ReleasesOwnership) {
    UniqueFd fd{::open("/dev/null", O_RDONLY)};
    ASSERT_TRUE(fd.valid());

    const int raw = fd.release();
    EXPECT_FALSE(fd.valid());
    EXPECT_GE(raw, 0);
    ::close(raw);
}

TEST(UniqueFd, MoveTransfersOwnership) {
    UniqueFd a{::open("/dev/null", O_RDONLY)};
    ASSERT_TRUE(a.valid());
    const int raw = a.get();

    UniqueFd b{std::move(a)};
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(b.get(), raw);
}

TEST(UniqueFd, ResetClosesPrevious) {
    int fds[2];
    ASSERT_EQ(make_pipe_fd(fds), 0);

    UniqueFd fd{fds[0]};
    fd.reset(fds[1]);

    // 旧的 fd 应该已经被关闭：对它操作会失败
    EXPECT_EQ(::fcntl(fds[0], F_GETFD), -1);
}
