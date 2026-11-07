// chat_hub.h — 聊天业务核心
//
// 所属层：chat
// 被谁使用：app::ChatServer（唯一调用者）
//
// 职责：维护用户表、昵称索引、房间成员，处理协议帧，产出"要发出去的东西"。
//
// 不持有的东西：fd、socket、Connection——一个都没有。
// 因此它可以纯逻辑测试：喂 Frame，断言 vector<Outbound>。
//
// 线程安全：**不是**线程安全的。调用方（app 层）负责用锁保护，
// 因为多线程模式下它会被工作线程池并发访问。

#pragma once

#include "chatroom/chat/types.h"
#include "chatroom/protocol/frame.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace chatroom::chat {

/// 聊天室业务核心。不是线程安全的。
class ChatHub {
public:
    /// 默认房间，客户端 JOIN 之后自动落在里面。
    static constexpr const char* kDefaultRoom = "lobby";

    ChatHub();

    /// 新连接进来：分配会话号，并产生一条欢迎消息。
    ///
    /// @param peer 对端地址，仅用于日志
    /// @param out  产出消息追加到这里（调用方负责发送）
    /// @return     新分配的会话号
    SessionId add_session(std::string peer, std::vector<Outbound>& out);

    /// 连接断开：清理昵称索引与房间成员关系，并通知房间里其他人。
    void remove_session(SessionId id, std::vector<Outbound>& out);

    /// 处理一条来自该会话的帧，结果追加到 out。
    ///
    /// 这是业务层的总入口：按 frame.type 分发到各个 handle_*。
    /// 未 JOIN 的会话除 JOIN 外的操作一律回错误，因为广播时需要昵称标识来源。
    void handle(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);

    std::size_t session_count() const noexcept {
        return users_.size();
    }

    std::size_t room_count() const noexcept {
        return rooms_.size();
    }

    bool has_user(SessionId id) const noexcept {
        return users_.find(id) != users_.end();
    }

    /// 查询用户信息；不存在返回 nullptr。
    const UserInfo* find_user(SessionId id) const;

private:
    void handle_join(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_nick(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_msg(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_dm(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_join_room(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_leave_room(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_list(SessionId id, std::vector<Outbound>& out);

    /// 构造一条 SYS 通知。
    static Outbound system(SessionId to, const char* code, std::string text);

    /// 向房间内成员投递（except 用于排除发送者自己）。
    void broadcast_room(const std::string& room, const protocol::Frame& frame, SessionId except,
                        std::vector<Outbound>& out);

    SessionId next_id_ = 1;
    std::unordered_map<SessionId, UserInfo> users_;
    std::unordered_map<std::string, SessionId> by_nickname_;  ///< 昵称唯一性靠它保证
    std::map<std::string, std::set<SessionId>> rooms_;        ///< 房间名 → 成员集合
};

}  // namespace chatroom::chat
