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

add_executable(chatroom_tests
    # 单元测试：不需要网络
    unit/test_buffer.cpp
    unit/test_chat_hub.cpp
    unit/test_frame_codec.cpp
    unit/test_thread_pool.cpp
    unit/test_unique_fd.cpp
    unit/test_version.cpp

    # 集成测试：在进程内起真实服务器，用真实 socket 通信
    integration/test_chat_flow.cpp
)

target_include_directories(chatroom_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(chatroom_tests PRIVATE
    chatroom_app
    chatroom_chat
    chatroom_core
    chatroom_infra
    chatroom_net
    chatroom_protocol
    GTest::gtest_main
)

# 让 ctest 自动发现每个 TEST()，不用手写 add_test。
include(GoogleTest)
gtest_discover_tests(chatroom_tests)
