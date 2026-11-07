#pragma once

namespace chatroom::platform {

// 文件描述符的 RAII 包装。
//
// 网络程序第一大类 bug 就是 fd 泄漏：某条错误分支 return 之前忘了 close，
// 进程的 fd 越积越多，最后 accept 返回 EMFILE，整个服务停摆。
//
// 让析构函数负责 close，可以让"忘记释放"变成不可能。move-only 保证
// 同一时刻只有一个对象拥有这个 fd，不会被 double close。
class UniqueFd {
public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    ~UniqueFd();

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept;
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    int get() const noexcept { return fd_; }

    bool valid() const noexcept { return fd_ >= 0; }

    explicit operator bool() const noexcept { return valid(); }

    // 放弃所有权：把一个已经创建的 fd 移交给别的机制管理时使用
    // （例如 accept 之后要把它存进 map）。
    int release() noexcept;

    void reset(int fd = -1) noexcept;

private:
    int fd_ = -1;
};

}  // namespace chatroom::platform
