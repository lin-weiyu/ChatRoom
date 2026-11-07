cmake_minimum_required(VERSION 3.20)

project(ChatRoom
    VERSION 1.0.0
    DESCRIPTION "Multi-client chat server in modern C++"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# 全局语言设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 没指定构建类型时默认 Debug：教学阶段需要断点和断言。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo)
endif()

# ---------------------------------------------------------------------------
# 选项
# ---------------------------------------------------------------------------
option(CHATROOM_BUILD_TESTS "Build unit and integration tests" ON)
option(CHATROOM_WERROR     "Treat warnings as errors (enabled in CI)" OFF)
option(CHATROOM_ASAN       "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(CHATROOM_TSAN       "Build with ThreadSanitizer" OFF)

# 只构建平台无关的目标。
#
# 存在的意义：epoll 只存在于 Linux 内核，net/app 层在 macOS 上编译不过。
# 打开这个开关后，CI 的 macOS job 仍然会编译并测试 protocol/chat——
# 如果哪天有人往业务层里偷偷 include 了 <sys/epoll.h>，那个 job 会立刻变红。
# 这就是"分层"的自动检查。
option(CHATROOM_PORTABLE_ONLY "Build only platform-independent targets" OFF)

if(CHATROOM_ASAN AND CHATROOM_TSAN)
    message(FATAL_ERROR "CHATROOM_ASAN and CHATROOM_TSAN are mutually exclusive")
endif()

# ---------------------------------------------------------------------------
# 全局编译选项
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings)

if(CHATROOM_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(CHATROOM_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# 目标：分层结构
#
# 依赖方向只能向下、且只能相邻：
#
#   chat -> protocol -> net -> platform
#
# 分层不是靠文档自觉，而是靠链接关系强制：net 链接不到 chat 的符号。
# ---------------------------------------------------------------------------

add_library(chatroom_platform STATIC
    src/platform/sys_error.cpp
    src/platform/unique_fd.cpp
)
target_include_directories(chatroom_platform PUBLIC include)
target_link_libraries(chatroom_platform PUBLIC chatroom_warnings Threads::Threads)

# 网络层：epoll、连接、缓冲区。Linux 专属，portable 模式下不构建。
if(NOT CHATROOM_PORTABLE_ONLY)
    add_library(chatroom_net STATIC
        src/net/acceptor.cpp
        src/net/buffer.cpp
        src/net/connection.cpp
        src/net/event_fd.cpp
        src/net/poller.cpp
        src/net/server.cpp
        src/net/socket.cpp
    )
    target_link_libraries(chatroom_net PUBLIC chatroom_platform)
endif()

# 协议层：纯函数编解码，不认识 socket。
add_library(chatroom_protocol STATIC
    src/protocol/frame_codec.cpp
)
target_link_libraries(chatroom_protocol PUBLIC chatroom_platform)

# 版本信息与将来的业务层
add_library(chatroom_core STATIC
    src/core/version.cpp
)
target_include_directories(chatroom_core PUBLIC include)
target_link_libraries(chatroom_core PUBLIC chatroom_platform)

# 版本号只有一个来源：project(VERSION ...)。用编译定义注入，而不是在源码里再写一遍。
target_compile_definitions(chatroom_core PRIVATE
    CHATROOM_VERSION="${PROJECT_VERSION}"
    CHATROOM_BUILD_TYPE="$<CONFIG>"
)

# 基础设施：日志、配置、线程池。平台无关，portable 模式下也构建。
add_library(chatroom_infra STATIC
    src/infra/config.cpp
    src/infra/logger.cpp
    src/infra/thread_pool.cpp
)
target_link_libraries(chatroom_infra PUBLIC chatroom_platform)

# 业务层：用户、房间、消息路由。不认识 socket。
add_library(chatroom_chat STATIC
    src/chat/chat_hub.cpp
)
target_link_libraries(chatroom_chat PUBLIC chatroom_protocol)

if(NOT CHATROOM_PORTABLE_ONLY)
    # 装配层：唯一同时认识 net::Connection 和 chat::ChatHub 的地方。
    add_library(chatroom_app STATIC
        src/app/chat_server.cpp
    )
    target_link_libraries(chatroom_app PUBLIC chatroom_net chatroom_chat chatroom_infra)

    add_executable(chatroom_server
        src/main.cpp
    )
    target_link_libraries(chatroom_server PRIVATE chatroom_app chatroom_core)

    add_executable(chatroom_loadgen
        tools/loadgen.cpp
    )
    target_link_libraries(chatroom_loadgen PRIVATE chatroom_protocol Threads::Threads)
endif()

# 客户端只用协议层，永远可以构建——它也是 macOS 上验证协议可移植性的证据。
add_executable(chatroom_cli
    tools/chat_cli.cpp
)
target_link_libraries(chatroom_cli PRIVATE chatroom_protocol)

# ---------------------------------------------------------------------------
# 测试
# ---------------------------------------------------------------------------
include(CTest)

if(CHATROOM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# 安装
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
if(CHATROOM_PORTABLE_ONLY)
    install(TARGETS chatroom_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(TARGETS chatroom_server chatroom_loadgen chatroom_cli
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
