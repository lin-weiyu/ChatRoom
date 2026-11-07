#!/usr/bin/env bash
#
# 在本机（WSL / Linux）直接构建并跑测试。最快的一条路。
#
#   ./scripts/build.sh              # Debug 构建 + 测试
#   ./scripts/build.sh Release      # Release 构建 + 测试
#   ./scripts/build.sh Debug asan   # 带 ASan/UBSan

set -euo pipefail

BUILD_TYPE="${1:-Debug}"
SANITIZER="${2:-}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

EXTRA_FLAGS=()
case "$SANITIZER" in
    asan) EXTRA_FLAGS+=(-DCHATROOM_ASAN=ON) ;;
    tsan) EXTRA_FLAGS+=(-DCHATROOM_TSAN=ON) ;;
    "")   ;;
    *)    echo "unknown sanitizer: $SANITIZER (expected asan|tsan)" >&2; exit 2 ;;
esac

cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    "${EXTRA_FLAGS[@]}"

cmake --build build --parallel

ctest --test-dir build --output-on-failure
