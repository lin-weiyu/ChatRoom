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

    if (config_.worker_threads > 0) {
        pool_ = std::make_unique<infra::ThreadPool>(
            static_cast<std::size_t>(config_.worker_threads));
        server_.set_wakeup_fd(wakeup_.fd(), [this] {
            wakeup_.drain();
            drain_results();
        });
        infra::log_info("worker threads: " + std::to_string(config_.worker_threads));
    } else {
        infra::log_info("worker threads: 0 (all business logic runs on the IO thread)");
    }
}

std::size_t ChatServer::session_count() const noexcept {
    const std::lock_guard<std::mutex> lock(hub_mutex_);
    return hub_.session_count();
}

void ChatServer::on_open(net::Connection& conn) {
    {
        const std::lock_guard<std::mutex> lock(hub_mutex_);

        // 连接数上限：满了就直接拒绝，而不是让 fd 一路涨到 EMFILE。
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
    }

    std::vector<chat::Outbound> outbounds;
    chat::SessionId id = chat::kInvalidSession;
    {
        const std::lock_guard<std::mutex> lock(hub_mutex_);
        id = hub_.add_session(conn.peer(), outbounds);
    }

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
        auto result = protocol::try_decode(conn.input().view());

        if (result.status == protocol::DecodeStatus::kIncomplete) {
            return;
        }
        if (result.status == protocol::DecodeStatus::kError) {
            infra::log_warn("protocol error from " + conn.peer() + ": " + result.error);
            conn.shutdown();
            return;
        }

        conn.input().retrieve(result.consumed);

        if (pool_ == nullptr) {
            // 单线程模式：直接在这里处理
            std::vector<chat::Outbound> outbounds;
            {
                const std::lock_guard<std::mutex> lock(hub_mutex_);
                hub_.handle(id, result.frame, outbounds);
            }
            dispatch(outbounds);
        } else {
            // 多线程模式：把帧丢给工作线程。注意这里**只传值**，
            // 绝不把 Connection* 传出去——那是 IO 线程的领地。
            protocol::Frame frame = std::move(result.frame);
            pool_->submit([this, id, frame = std::move(frame)]() mutable {
                handle_task(id, std::move(frame));
            });
        }

        if (conn.closed()) {
            return;
        }
    }
}

void ChatServer::handle_task(chat::SessionId id, protocol::Frame frame) {
    std::vector<chat::Outbound> outbounds;
    {
        const std::lock_guard<std::mutex> lock(hub_mutex_);
        hub_.handle(id, frame, outbounds);
    }

    if (!outbounds.empty()) {
        const std::lock_guard<std::mutex> lock(result_mutex_);
        for (chat::Outbound& out : outbounds) {
            results_.push_back(std::move(out));
        }
    }

    // 叫醒 IO 线程。没有这一步，结果要等到下一次 epoll_wait 超时才被处理。
    wakeup_.notify();
}

void ChatServer::drain_results() {
    std::vector<chat::Outbound> local;
    {
        const std::lock_guard<std::mutex> lock(result_mutex_);
        local.swap(results_);
    }
    if (!local.empty()) {
        dispatch(local);
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
    {
        const std::lock_guard<std::mutex> lock(hub_mutex_);
        hub_.remove_session(id, outbounds);
    }
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
    if (pool_ != nullptr) {
        drain_results();
    }
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
