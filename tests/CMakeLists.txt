# 测试目标。
#
# GTest 两条路径：
#   - 系统里装了 GTest（apt/brew）就直接用，快且离线可用；
#   - 没装就 FetchContent 拉源码，保证任何机器都能跑。
# 这种 "先 find_package，再 fallback" 是真实项目的常见写法。

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# 平台无关的用例：协议、业务、线程池。
set(CHATROOM_TEST_SOURCES
    unit/test_chat_hub.cpp
    unit/test_frame_codec.cpp
    unit/test_thread_pool.cpp
    unit/test_unique_fd.cpp
    unit/test_version.cpp
)

# Linux 专属的用例：缓冲区、eventfd、真实 socket 集成测试。
if(NOT CHATROOM_PORTABLE_ONLY)
    list(APPEND CHATROOM_TEST_SOURCES
        unit/test_buffer.cpp
        unit/test_event_fd.cpp
        integration/test_chat_flow.cpp
    )
endif()

add_executable(chatroom_tests ${CHATROOM_TEST_SOURCES})

target_include_directories(chatroom_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(chatroom_tests PRIVATE
    chatroom_chat
    chatroom_core
    chatroom_infra
    chatroom_protocol
    GTest::gtest_main
)

if(NOT CHATROOM_PORTABLE_ONLY)
    target_link_libraries(chatroom_tests PRIVATE chatroom_app chatroom_net)
endif()

# 让 ctest 自动发现每个 TEST()，不用手写 add_test。
include(GoogleTest)
gtest_discover_tests(chatroom_tests)
