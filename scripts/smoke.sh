#!/usr/bin/env bash
#
# 冒烟测试：起一个真实的服务器，用 netcat 连上去，验证字节能原样回来。
#
# 为什么不放在 ctest 里：它依赖系统的 nc，且需要端口。真正的自动化集成
# 测试（M7）会用 C++ 自己实现客户端，不依赖外部工具。
#
#   ./scripts/smoke.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BIN="${BIN:-./build/chatroom_server}"
PORT="${PORT:-12345}"
LOG="${TMPDIR:-/tmp}/chatroom-smoke.log"

if [[ ! -x "$BIN" ]]; then
    echo "server binary not found: $BIN (run ./scripts/build.sh first)" >&2
    exit 1
fi

"$BIN" > "$LOG" 2>&1 &
SRV_PID=$!
trap 'kill "$SRV_PID" 2>/dev/null || true' EXIT

# 等端口就绪，最多 3 秒
for _ in $(seq 1 30); do
    if nc -z 127.0.0.1 "$PORT" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

if ! kill -0 "$SRV_PID" 2>/dev/null; then
    echo "FAIL: server exited immediately" >&2
    cat "$LOG" >&2
    exit 1
fi

ACTUAL=$(printf 'hello\n' | nc -q 1 127.0.0.1 "$PORT" || true)

if [[ "$ACTUAL" != "hello" ]]; then
    echo "FAIL: expected 'hello', got '$ACTUAL'" >&2
    echo "--- server log ---" >&2
    cat "$LOG" >&2
    exit 1
fi

echo "OK: echo round-trip works"
