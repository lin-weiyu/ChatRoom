# syntax=docker/dockerfile:1
#
# 两阶段构建：
#   builder —— 装编译器、编译、跑测试；镜像很大，但不进入最终产物
#   runtime —— 只拷贝二进制，镜像小、攻击面小
#
# 这样做的直接收益：最终镜像 ~80MB 而不是 ~800MB。

# ---------------------------------------------------------------------------
# 阶段 1：构建 + 跑测试
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 依赖单独一层：只要这一层不变，改代码时不用重装工具链（层缓存）。
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        ca-certificates \
        libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# 先 COPY 再编译。COPY 之后的层会因代码改动全部失效，所以把"贵"的步骤放前面。
COPY . .

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCHATROOM_WERROR=ON \
    && cmake --build build --parallel \
    && ctest --test-dir build --output-on-failure

# ---------------------------------------------------------------------------
# 阶段 2：运行时
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --shell /usr/sbin/nologin --uid 10001 chatroom

COPY --from=builder /src/build/chatroom_server /usr/local/bin/chatroom_server

# 不以 root 跑服务：容器逃逸时攻击面小得多。
USER chatroom

EXPOSE 12345

ENTRYPOINT ["/usr/local/bin/chatroom_server"]
