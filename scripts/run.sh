#!/usr/bin/env bash
#
# 起服务器。默认用 Docker，端口 12345 映射到宿主机。
#
#   ./scripts/run.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

exec docker compose up --build
