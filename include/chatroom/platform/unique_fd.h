// unique_fd.h — 文件描述符的 RAII 包装
//
// 所属层：platform（最底层，只依赖标准库和 POSIX）
// 被谁使用：net 层的所有成员
//
// 为什么需要它：
//   网络程序第一大类 bug 是 fd 泄漏——某条错误分支 return 之前忘了 close，
//   进程 fd 数越积越多，最后 accept 返回 EMFILE，整个服务停摆。
//   把释放交给析构函数，可以让"忘记释放"变成不可能。

#pragma once

namespace chatroom::platform {

/// 独占所有权的文件描述符。
///
/// 契约：
///   - 同一时刻只有一个 UniqueFd 拥有这个 fd（move-only，禁止拷贝）
///   - 析构时自动 close()，如果持有有效 fd
///   - release() 放弃所有权，之后由调用方负责 close
///
/// 注意：close() 之后 fd 号可能被内核复用，所以**不要用 fd 做业务主键**
/// （见 chat::SessionId 的设计说明）。
class UniqueFd {
public:
    /// 构造一个"空"的包装，不持有任何 fd。
    UniqueFd() noexcept = default;

    /// 接管一个已存在的 fd 的所有权。不检查 fd 是否合法。
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    /// 如果持有有效 fd，关闭它。
    ~UniqueFd();

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    /// 转移所有权。other 变为无效。
    UniqueFd(UniqueFd&& other) noexcept;

    /// 先释放自己持有的 fd，再接管 other 的。
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    /// 返回原始 fd，-1 表示未持有。
    int get() const noexcept {
        return fd_;
    }

    bool valid() const noexcept {
        return fd_ >= 0;
    }

    explicit operator bool() const noexcept {
        return valid();
    }

    /// 放弃所有权并返回 fd，调用方负责关闭。
    /// 典型用途：accept 之后要把 fd 交给别的对象管理。
    int release() noexcept;

    /// 关闭当前 fd（如果有），改为持有新的 fd。传 -1 表示变为空。
    void reset(int fd = -1) noexcept;

private:
    int fd_ = -1;
};

}  // namespace chatroom::platform
