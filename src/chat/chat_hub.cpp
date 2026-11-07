#include "chatroom/chat/chat_hub.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace chatroom::chat {

namespace {

constexpr std::size_t kMaxNicknameLength = 32;

std::string field_or(const protocol::Frame& frame, std::size_t index) {
    return index < frame.fields.size() ? frame.fields[index] : std::string{};
}

}  // namespace

ChatHub::ChatHub() {
    // 默认房间常驻，客户端 JOIN 之后直接落在这里。
    rooms_[kDefaultRoom];
}

Outbound ChatHub::system(SessionId to, const char* code, std::string text) {
    Outbound out;
    out.to = to;
    out.frame.type = protocol::MessageType::kSys;
    out.frame.fields = {code, std::move(text)};
    return out;
}

const UserInfo* ChatHub::find_user(SessionId id) const {
    const auto it = users_.find(id);
    return it == users_.end() ? nullptr : &it->second;
}

SessionId ChatHub::add_session(std::string peer, std::vector<Outbound>& out) {
    const SessionId id = next_id_++;

    UserInfo info;
    info.id = id;
    info.peer = std::move(peer);
    users_.emplace(id, std::move(info));

    out.push_back(system(id, "info", "welcome, send JOIN <nickname> to enter the lobby"));
    return id;
}

void ChatHub::remove_session(SessionId id, std::vector<Outbound>& out) {
    const auto it = users_.find(id);
    if (it == users_.end()) {
        return;
    }
    const UserInfo& user = it->second;

    if (!user.nickname.empty()) {
        by_nickname_.erase(user.nickname);
    }

    const auto room_it = rooms_.find(user.room);
    if (room_it != rooms_.end()) {
        room_it->second.erase(id);
        if (!user.nickname.empty()) {
            broadcast_room(user.room,
                           protocol::Frame{protocol::MessageType::kSys,
                                           {"leave", user.nickname + " left " + user.room}},
                           id, out);
        }
    }

    users_.erase(it);
}

void ChatHub::broadcast_room(const std::string& room, const protocol::Frame& frame,
                             SessionId except, std::vector<Outbound>& out) {
    const auto it = rooms_.find(room);
    if (it == rooms_.end()) {
        return;
    }
    for (const SessionId member : it->second) {
        if (member == except) {
            continue;
        }
        Outbound msg;
        msg.to = member;
        msg.frame = frame;
        out.push_back(std::move(msg));
    }
}

void ChatHub::handle(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out) {
    const auto it = users_.find(id);
    if (it == users_.end()) {
        return;
    }

    // 除 JOIN 之外，一切操作都要求先有昵称——否则广播时无法标识来源。
    if (frame.type != protocol::MessageType::kJoin && it->second.nickname.empty()) {
        out.push_back(system(id, "error", "join first: JOIN <nickname>"));
        return;
    }

    switch (frame.type) {
        case protocol::MessageType::kJoin:
            handle_join(id, frame, out);
            break;
        case protocol::MessageType::kNick:
            handle_nick(id, frame, out);
            break;
        case protocol::MessageType::kMsg:
            handle_msg(id, frame, out);
            break;
        case protocol::MessageType::kDm:
            handle_dm(id, frame, out);
            break;
        case protocol::MessageType::kJoinRoom:
            handle_join_room(id, frame, out);
            break;
        case protocol::MessageType::kLeaveRoom:
            handle_leave_room(id, frame, out);
            break;
        case protocol::MessageType::kList:
            handle_list(id, out);
            break;
        case protocol::MessageType::kPing:
            out.push_back(system(id, "pong", ""));
            break;
        case protocol::MessageType::kQuit: {
            Outbound bye = system(id, "info", "bye");
            bye.close_after_send = true;
            out.push_back(std::move(bye));
            break;
        }
        default:
            out.push_back(system(id, "error", "unknown message type"));
            break;
    }
}

void ChatHub::handle_join(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out) {
    UserInfo& user = users_.at(id);
    const std::string nickname = field_or(frame, 0);

    if (nickname.empty() || nickname.size() > kMaxNicknameLength) {
        out.push_back(system(id, "error", "nickname must be 1..32 bytes"));
        return;
    }

    if (!user.nickname.empty() && user.nickname != nickname) {
        by_nickname_.erase(user.nickname);
    }

    const auto taken = by_nickname_.find(nickname);
    if (taken != by_nickname_.end() && taken->second != id) {
        out.push_back(system(id, "error", "nickname already in use"));
        return;
    }

    user.nickname = nickname;
    by_nickname_[nickname] = id;

    const std::string room = user.room.empty() ? kDefaultRoom : user.room;
    user.room = room;
    rooms_[room].insert(id);

    out.push_back(system(id, "info", "welcome, " + nickname + " (room: " + room + ")"));
    broadcast_room(
        room, protocol::Frame{protocol::MessageType::kSys, {"join", nickname + " joined " + room}},
        id, out);
}

void ChatHub::handle_nick(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out) {
    UserInfo& user = users_.at(id);
    const std::string nickname = field_or(frame, 0);

    if (nickname.empty() || nickname.size() > kMaxNicknameLength) {
        out.push_back(system(id, "error", "nickname must be 1..32 bytes"));
        return;
    }
    if (by_nickname_.count(nickname) != 0 && nickname != user.nickname) {
        out.push_back(system(id, "error", "nickname already in use"));
        return;
    }

    const std::string old_name = user.nickname;
    by_nickname_.erase(old_name);

    user.nickname = nickname;
    by_nickname_[nickname] = id;

    out.push_back(system(id, "info", "you are now " + nickname));
    broadcast_room(
        user.room,
        protocol::Frame{protocol::MessageType::kSys, {"info", old_name + " is now " + nickname}},
        id, out);
}

void ChatHub::handle_msg(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out) {
    const UserInfo& user = users_.at(id);

    const std::string requested_room = field_or(frame, 0);
    const std::string room = requested_room.empty() ? user.room : requested_room;
    const std::string text = field_or(frame, 1);

    if (text.empty()) {
        out.push_back(system(id, "error", "empty message"));
        return;
    }

    const auto it = rooms_.find(room);
    if (it == rooms_.end() || it->second.count(id) == 0) {
        out.push_back(system(id, "error", "you are not in room: " + room));
        return;
    }

    // 服务端把发送者昵称补进帧里，客户端才能显示 "alice: hello"。
    protocol::Frame broadcast;
    broadcast.type = protocol::MessageType::kMsg;
    broadcast.fields = {user.nickname, room, text};

    broadcast_room(room, broadcast, kInvalidSession, out);
}

void ChatHub::handle_dm(SessionId id, const protocol::Frame& frame, std::vector<Outbound>& out) {
    const UserInfo& user = users_.at(id);
    const std::string target = field_or(frame, 0);
    const std::string text = field_or(frame, 1);

    const auto it = by_nickname_.find(target);
    if (it == by_nickname_.end()) {
        out.push_back(system(id, "error", "no such user: " + target));
        return;
    }

    Outbound dm;
    dm.to = it->second;
    dm.frame.type = protocol::MessageType::kDm;
    dm.frame.fields = {user.nickname, text, "dm"};
    out.push_back(std::move(dm));
}

void ChatHub::handle_join_room(SessionId id, const protocol::Frame& frame,
                               std::vector<Outbound>& out) {
    UserInfo& user = users_.at(id);
    const std::string room = field_or(frame, 0);

    if (room.empty()) {
        out.push_back(system(id, "error", "room name required"));
        return;
    }
    if (user.room == room) {
        out.push_back(system(id, "info", "already in " + room));
        return;
    }

    rooms_[user.room].erase(id);
    broadcast_room(user.room,
                   protocol::Frame{protocol::MessageType::kSys,
                                   {"leave", user.nickname + " left " + user.room}},
                   id, out);

    user.room = room;
    rooms_[room].insert(id);

    out.push_back(system(id, "info", "joined " + room));
    broadcast_room(
        room,
        protocol::Frame{protocol::MessageType::kSys, {"join", user.nickname + " joined " + room}},
        id, out);
}

void ChatHub::handle_leave_room(SessionId id, const protocol::Frame& frame,
                                std::vector<Outbound>& out) {
    UserInfo& user = users_.at(id);
    const std::string room = field_or(frame, 0);

    if (room.empty() || room == kDefaultRoom) {
        out.push_back(system(id, "error", "cannot leave the default room"));
        return;
    }

    rooms_[room].erase(id);
    if (user.room == room) {
        user.room = kDefaultRoom;
        rooms_[kDefaultRoom].insert(id);
    }

    out.push_back(system(id, "info", "left " + room + ", back to " + kDefaultRoom));
}

void ChatHub::handle_list(SessionId id, std::vector<Outbound>& out) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& [name, members] : rooms_) {
        if (!first) {
            oss << ' ';
        }
        first = false;
        oss << name << '(' << members.size() << ')';
    }
    out.push_back(system(id, "info", "rooms: " + oss.str()));
}

}  // namespace chatroom::chat
