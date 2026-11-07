#!/usr/bin/env bash
#
# 压测：起服务器，跑 loadgen，打印吞吐。
#
#   ./scripts/bench.sh [connections] [messages_per_conn]

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CONNECTIONS="${1:-200}"
MESSAGES="${2:-20}"
PORT="${PORT:-12348}"
LOG="${TMPDIR:-/tmp}/chatroom-bench.log"

./build/chatroom_server --port "$PORT" > "$LOG" 2>&1 &
SRV=$!
trap 'kill "$SRV" 2>/dev/null || true' EXIT

for _ in $(seq 1 50); do nc -z 127.0.0.1 "$PORT" 2>/dev/null && break; sleep 0.1; done

./build/chatroom_loadgen 127.0.0.1 "$PORT" "$CONNECTIONS" "$MESSAGES"
