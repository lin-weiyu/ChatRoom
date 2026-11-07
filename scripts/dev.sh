#!/usr/bin/env bash
#
# 用 Docker 构建并跑测试——和 CI 完全相同的环境。
#
# 什么时候用它：
#   - 你在 macOS / 别的 Linux 发行版上，没有本项目的工具链
#   - 想确认"我本地能过，CI 为什么挂"（答案通常是环境差异，这里消除它）
#
#   ./scripts/dev.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

docker build --target builder -t chatroom-dev:latest .
docker run --rm chatroom-dev:latest
