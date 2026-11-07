#include "chatroom/app/chat_server.h"

#include <utility>
#include <vector>

#include "chatroom/protocol/frame_codec.h"

namespace chatroom::app {

ChatServer::ChatServer(std::uint16_t port) : server_(port) {
    server_.set_open_handler([this](net::Connection& conn) { on_open(conn); });
    server_.set_message_handler(
        [this](net::Connection& conn, std::string_view data) { on_message(conn, data); });
    server_.set_close_handler([this](net::Connection& conn) { on_close(conn); });
}

void ChatServer::on_open(net::Connection& conn) {
    std::vector<chat::Outbound> outbounds;
    const chat::SessionId id = hub_.add_session(conn.peer(), outbounds);

    fd_to_session_[conn.fd()] = id;
    session_to_conn_[id] = &conn;

    dispatch(outbounds);
}

void ChatServer::on_message(net::Connection& conn, std::string_view /*data*/) {
    const auto it = fd_to_session_.find(conn.fd());
    if (it == fd_to_session_.end()) {
        return;
    }
    const chat::SessionId id = it->second;

    // 循环取帧：一次 recv 可能带来半个帧（等下一次），也可能带来三个帧
    // （全部处理完）。这正是长度前缀协议的价值。
    for (;;) {
        const auto result = protocol::try_decode(conn.input().view());

        if (result.status == protocol::DecodeStatus::kIncomplete) {
            return;
        }
        if (result.status == protocol::DecodeStatus::kError) {
            // 字节流已经不可信，继续读只会读到垃圾。直接断开。
            conn.shutdown();
            return;
        }

        conn.input().retrieve(result.consumed);

        std::vector<chat::Outbound> outbounds;
        hub_.handle(id, result.frame, outbounds);
        dispatch(outbounds);

        if (conn.closed()) {
            return;
        }
    }
}

void ChatServer::on_close(net::Connection& conn) {
    const auto it = fd_to_session_.find(conn.fd());
    if (it == fd_to_session_.end()) {
        return;
    }

    const chat::SessionId id = it->second;
    fd_to_session_.erase(it);
    session_to_conn_.erase(id);

    std::vector<chat::Outbound> outbounds;
    hub_.remove_session(id, outbounds);
    dispatch(outbounds);
}

void ChatServer::dispatch(const std::vector<chat::Outbound>& outbounds) {
    for (const chat::Outbound& out : outbounds) {
        const auto it = session_to_conn_.find(out.to);
        if (it == session_to_conn_.end() || it->second == nullptr) {
            continue;  // 目标已经断开，丢弃即可
        }

        net::Connection& conn = *it->second;
        if (conn.closed()) {
            continue;
        }

        const std::string bytes = protocol::encode(out.frame);
        if (bytes.empty()) {
            continue;
        }
        conn.send(bytes);

        if (out.close_after_send) {
            // 只关写方向，让对端先把最后的回包读完再断开。
            // 直接 close 会触发 RST，把刚发出去的数据一起丢掉。
            conn.shutdown_write();
        }
    }
}

void ChatServer::run_once(int timeout_ms) { server_.run_once(timeout_ms); }

}  // namespace chatroom::app
