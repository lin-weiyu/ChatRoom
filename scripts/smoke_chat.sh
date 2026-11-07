#!/usr/bin/env bash
#
# 端到端冒烟：起服务器，用 chat_cli 走一遍 加入 -> 说话 -> 退出。
#
#   ./scripts/smoke_chat.sh

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BIN="${BIN:-./build/chatroom_server}"
CLI="${CLI:-./build/chatroom_cli}"
PORT="${PORT:-12346}"
LOG="${TMPDIR:-/tmp}/chatroom-smoke-chat.log"

echo "[smoke] starting $BIN on port $PORT"
"$BIN" "$PORT" > "$LOG" 2>&1 &
SRV_PID=$!
trap 'kill "$SRV_PID" 2>/dev/null || true' EXIT

READY=0
for _ in $(seq 1 50); do
    if nc -z 127.0.0.1 "$PORT" 2>/dev/null; then
        READY=1
        break
    fi
    sleep 0.1
done

if [[ "$READY" != "1" ]]; then
    echo "FAIL: server did not start" >&2
    cat "$LOG" >&2
    exit 1
fi

echo "[smoke] server is up, running client"
OUT=$(printf '/join alice\n/msg hello world\n/quit\n' | timeout 10 "$CLI" 127.0.0.1 "$PORT" 2>&1)
RC=$?
echo "[smoke] client exit code: $RC"
echo "--- client output ---"
echo "$OUT"
echo "--- server log ---"
cat "$LOG"

if [[ "$RC" != "0" ]]; then
    echo "FAIL: client exited with $RC" >&2
    exit 1
fi

grep -q "welcome" <<< "$OUT" || { echo "FAIL: no welcome message" >&2; exit 1; }
grep -q "alice" <<< "$OUT" || { echo "FAIL: join did not set nickname" >&2; exit 1; }
grep -q "hello world" <<< "$OUT" || { echo "FAIL: message was not broadcast back" >&2; exit 1; }

echo "OK: join + broadcast round-trip works"
