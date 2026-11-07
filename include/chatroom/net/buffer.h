#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace chatroom::net {

// 可增长的字节缓冲区。
//
// 核心设计：读写各用一个下标，retrieve() 只移动读下标，不搬运数据。
// 只有在"尾部空间不够、但头部空间可以回收"时才整体压缩一次。
//
// 为什么不用 std::string 拼接？因为每次 recv 之后 erase(0, n) 是 O(n)
// 的内存搬移，一个高频短消息的服务会因此把 CPU 全烧在 memmove 上。
class Buffer {
public:
    static constexpr std::size_t kInitialCapacity = 1024;

    Buffer() : buf_(kInitialCapacity) {}

    void append(const char* data, std::size_t len);
    void append(std::string_view data) {
        append(data.data(), data.size());
    }

    // 可读区间的起始指针与长度
    const char* data() const noexcept {
        return buf_.data() + read_index_;
    }
    std::size_t readable() const noexcept {
        return write_index_ - read_index_;
    }

    // 消费掉前 n 字节。n 大于可读长度时按可读长度处理。
    void retrieve(std::size_t n) noexcept;

    std::string_view view() const noexcept {
        return {data(), readable()};
    }

private:
    // 保证尾部至少能再写 len 字节
    void ensure_writable(std::size_t len);

    // 把 [read_index_, write_index_) 搬到起点
    void compact() noexcept;

    std::vector<char> buf_;
    std::size_t read_index_ = 0;
    std::size_t write_index_ = 0;
};

}  // namespace chatroom::net
