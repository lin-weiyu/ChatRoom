cmake_minimum_required(VERSION 3.20)

project(ChatRoom
    VERSION 0.1.0
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

# 网络层：epoll、连接、缓冲区。Linux 专属。
add_library(chatroom_net STATIC
    src/net/acceptor.cpp
    src/net/buffer.cpp
    src/net/connection.cpp
    src/net/poller.cpp
    src/net/server.cpp
    src/net/socket.cpp
)
target_link_libraries(chatroom_net PUBLIC chatroom_platform)

# 版本信息与将来的协议/业务层
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

add_executable(chatroom_server
    src/main.cpp
)
target_link_libraries(chatroom_server PRIVATE chatroom_core chatroom_net)

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
install(TARGETS chatroom_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
