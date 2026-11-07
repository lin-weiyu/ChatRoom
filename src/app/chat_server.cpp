#include "chatroom/app/chat_server.h"

#include <utility>
#include <vector>

#include "chatroom/infra/logger.h"
#include "chatroom/protocol/frame_codec.h"

namespace chatroom::app {

ChatServer::ChatServer(const infra::Config& config)
    : config_(config), server_(config.port, config.backlog) {
    server_.set_open_handler([this](net::Connection& conn) { on_open(conn); });
    server_.set_message_handler(
        [this](net::Connection& conn, std::string_view data) { on_message(conn, data); });
    server_.set_close_handler([this](net::Connection& conn) { on_close(conn); });
}

void ChatServer::on_open(net::Connection& conn) {
    // 连接数上限：满了就直接拒绝，而不是让 fd 一路涨到 EMFILE。
    // 真实项目里会回一个"服务器忙"的协议消息再断开。
    if (config_.max_connections > 0 &&
        static_cast<int>(hub_.session_count()) >= config_.max_connections) {
        chat::Outbound reject;
        reject.to = chat::kInvalidSession;
        reject.frame.type = protocol::MessageType::kSys;
        reject.frame.fields = {"error", "server full, try later"};
        const std::string bytes = protocol::encode(reject.frame);
        conn.send(bytes);
        conn.shutdown_write();
        infra::log_warn("rejecting connection: max_connections reached");
        return;
    }

    std::vector<chat::Outbound> outbounds;
    const chat::SessionId id = hub_.add_session(conn.peer(), outbounds);

    fd_to_session_[conn.fd()] = id;
    session_to_conn_[id] = &conn;
    last_active_[id] = Clock::now();

    infra::log_info("connection opened: " + conn.peer() + " (session " + std::to_string(id) + ")");
    dispatch(outbounds);
}

void ChatServer::on_message(net::Connection& conn, std::string_view /*data*/) {
    const auto it = fd_to_session_.find(conn.fd());
    if (it == fd_to_session_.end()) {
        return;
    }
    const chat::SessionId id = it->second;
    last_active_[id] = Clock::now();

    // 循环取帧：一次 recv 可能带来半个帧（等下一次），也可能带来三个帧
    // （全部处理完）。这正是长度前缀协议的价值。
    for (;;) {
        const auto result = protocol::try_decode(conn.input().view());

        if (result.status == protocol::DecodeStatus::kIncomplete) {
            return;
        }
        if (result.status == protocol::DecodeStatus::kError) {
            // 字节流已经不可信，继续读只会读到垃圾。直接断开。
            infra::log_warn("protocol error from " + conn.peer() + ": " + result.error);
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
    last_active_.erase(id);

    infra::log_info("connection closed: " + conn.peer() + " (session " + std::to_string(id) + ")");

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
            infra::log_debug("closing after send: session " + std::to_string(out.to));
            conn.shutdown_write();
        }
    }
}

void ChatServer::check_idle() {
    if (config_.idle_timeout_seconds <= 0) {
        return;
    }

    const auto now = Clock::now();
    const auto limit = std::chrono::seconds(config_.idle_timeout_seconds);

    std::vector<chat::SessionId> expired;
    for (const auto& [id, last] : last_active_) {
        if (now - last > limit) {
            expired.push_back(id);
        }
    }

    for (const chat::SessionId id : expired) {
        const auto it = session_to_conn_.find(id);
        if (it == session_to_conn_.end()) {
            continue;
        }
        net::Connection& conn = *it->second;
        chat::Outbound bye;
        bye.to = id;
        bye.frame.type = protocol::MessageType::kSys;
        bye.frame.fields = {"error", "idle timeout"};
        const std::string bytes = protocol::encode(bye.frame);
        conn.send(bytes);
        conn.shutdown_write();
        infra::log_info("idle timeout: session " + std::to_string(id));
    }
}

void ChatServer::run_once(int timeout_ms) {
    server_.run_once(timeout_ms);
    check_idle();
}

void ChatServer::shutdown() {
    for (auto& [id, conn] : session_to_conn_) {
        if (conn == nullptr || conn->closed()) {
            continue;
        }
        chat::Outbound bye;
        bye.to = id;
        bye.frame.type = protocol::MessageType::kSys;
        bye.frame.fields = {"info", "server shutting down"};
        const std::string bytes = protocol::encode(bye.frame);
        conn->send(bytes);
        conn->shutdown_write();
    }
    infra::log_info("server shutdown requested");
}

}  // namespace chatroom::app
