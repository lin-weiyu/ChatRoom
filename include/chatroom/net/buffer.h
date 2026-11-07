// buffer.h — 可增长的字节缓冲区
//
// 所属层：net（只认识字节，不认识"消息"）
// 被谁使用：Connection（收发缓冲）、ChatServer（解析协议帧）
//
// 核心设计：读写下标分离。
//   [ 已丢弃 | 可读 data()..readable() | 可写空间 ]
//            ^read_index_              ^write_index_
//
// retrieve() 只移动读下标，不搬运数据。只有当"尾部空间不够、但头部空间
// 可以回收"时才整体压缩一次。
//
// 为什么不用 std::string 拼接：
//   每次 recv 之后 erase(0, n) 都是 O(n) 的内存搬移。一个高频短消息的
//   服务会因此把 CPU 全烧在 memmove 上。

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace chatroom::net {

/// 字节缓冲区。不是线程安全的——它只在 IO 线程里被访问。
class Buffer {
public:
    static constexpr std::size_t kInitialCapacity = 1024;

    Buffer() : buf_(kInitialCapacity) {}

    /// 在尾部追加 len 字节，必要时扩容。
    void append(const char* data, std::size_t len);

    /// 追加一段字符串视图。
    void append(std::string_view data) {
        append(data.data(), data.size());
    }

    /// 可读区间的起始指针。可能因为后续 append 而失效。
    const char* data() const noexcept {
        return buf_.data() + read_index_;
    }

    /// 当前可读字节数。
    std::size_t readable() const noexcept {
        return write_index_ - read_index_;
    }

    /// 消费掉前 n 字节。n 超过可读长度时按可读长度处理（防御性设计）。
    void retrieve(std::size_t n) noexcept;

    /// 可读区间，用于交给协议解析器。
    std::string_view view() const noexcept {
        return {data(), readable()};
    }

private:
    /// 保证尾部至少还能写 len 字节：先尝试压缩，实在不行才扩容。
    void ensure_writable(std::size_t len);

    /// 把 [read_index_, write_index_) 搬到起点，回收头部空洞。
    void compact() noexcept;

    std::vector<char> buf_;
    std::size_t read_index_ = 0;
    std::size_t write_index_ = 0;
};

}  // namespace chatroom::net
