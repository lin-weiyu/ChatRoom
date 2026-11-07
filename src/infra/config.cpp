#include "chatroom/infra/config.h"

#include <cstdlib>
#include <cstring>
#include <sstream>

#include "chatroom/version.h"

namespace chatroom::infra {

namespace {

bool parse_int(const char* text, long& out) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    out = value;
    return true;
}

}  // namespace

std::string usage(const char* program) {
    std::ostringstream oss;
    oss << "usage: " << program << " [options]\n"
        << "\n"
        << "  --port <n>          listen port (default 12345, 0 = kernel-assigned)\n"
        << "  --max-conns <n>     reject new connections beyond this limit (default 1024)\n"
        << "  --idle-timeout <s>  disconnect silent clients after s seconds (0 = off, default 120)\n"
        << "  --workers <n>       business-logic worker threads (0 = run on the IO thread, default 2)\n"
        << "  --backlog <n>       listen backlog (default 128)\n"
        << "  --verbose           enable debug logs\n"
        << "  --version           print version and exit\n"
        << "  --help              print this help and exit\n";
    return oss.str();
}

ParseOutcome parse_args(int argc, char** argv) {
    ParseOutcome outcome;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            outcome.ok = true;
            outcome.should_exit = true;
            outcome.message = usage(argv[0]);
            return outcome;
        }
        if (arg == "--version") {
            outcome.ok = true;
            outcome.should_exit = true;
            outcome.message = std::string("chatroom ") + std::string(chatroom::version());
            return outcome;
        }
        if (arg == "--verbose") {
            outcome.config.verbose = true;
            continue;
        }

        // 所有带参数的选项都要求后面还有一个值
        if (i + 1 >= argc) {
            outcome.ok = false;
            outcome.message = "missing value for " + arg;
            return outcome;
        }

        long value = 0;
        if (!parse_int(argv[i + 1], value)) {
            outcome.ok = false;
            outcome.message = "invalid number for " + arg + ": " + argv[i + 1];
            return outcome;
        }
        ++i;

        if (arg == "--port") {
            if (value < 0 || value > 65535) {
                outcome.ok = false;
                outcome.message = "port out of range";
                return outcome;
            }
            outcome.config.port = static_cast<std::uint16_t>(value);
        } else if (arg == "--max-conns") {
            outcome.config.max_connections = static_cast<int>(value);
        } else if (arg == "--idle-timeout") {
            outcome.config.idle_timeout_seconds = static_cast<int>(value);
        } else if (arg == "--workers") {
            outcome.config.worker_threads = static_cast<int>(value);
        } else if (arg == "--backlog") {
            outcome.config.backlog = static_cast<int>(value);
        } else {
            outcome.ok = false;
            outcome.message = "unknown option: " + arg;
            return outcome;
        }
    }

    outcome.ok = true;
    return outcome;
}

}  // namespace chatroom::infra
