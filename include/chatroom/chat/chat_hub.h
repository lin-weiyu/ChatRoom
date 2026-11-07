#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "chatroom/chat/types.h"
#include "chatroom/protocol/frame.h"

namespace chatroom::chat {

// 聊天业务核心。
//
// 职责：维护用户表、房间表，处理协议帧，产出"要发出去的东西"。
// 不持有 fd、不调用 send、不做任何 I/O——因此可以纯逻辑测试。
class ChatHub {
public:
    static constexpr const char* kDefaultRoom = "lobby";

    ChatHub();

    // 新连接进来。返回会话号，并产生一条欢迎消息。
    SessionId add_session(std::string peer, std::vector<Outbound>& out);

    // 连接断开。清理房间成员关系，并通知房间里其他人。
    void remove_session(SessionId id, std::vector<Outbound>& out);

    // 处理一条来自该会话的帧。
    void handle(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);

    std::size_t session_count() const noexcept { return users_.size(); }

    std::size_t room_count() const noexcept { return rooms_.size(); }

    bool has_user(SessionId id) const noexcept { return users_.find(id) != users_.end(); }

    const UserInfo* find_user(SessionId id) const;

private:
    void handle_join(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_nick(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_msg(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_dm(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_join_room(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_leave_room(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out);
    void handle_list(SessionId id, std::vector<Outbound>& out);

    // 构造一条 SYS 消息
    static Outbound system(SessionId to, const char* code, std::string text);

    // 向某房间的成员投递（except 用于排除发送者）
    void broadcast_room(const std::string& room, const protocol::Frame& frame, SessionId except,
                        std::vector<Outbound>& out);

    SessionId next_id_ = 1;
    std::unordered_map<SessionId, UserInfo> users_;
    std::unordered_map<std::string, SessionId> by_nickname_;
    std::map<std::string, std::set<SessionId>> rooms_;
};

}  // namespace chatroom::chat
