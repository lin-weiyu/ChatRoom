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
    unit/test_version.cpp
)

target_link_libraries(chatroom_tests PRIVATE
    chatroom_core
    GTest::gtest_main
)

# 让 ctest 自动发现每个 TEST()，不用手写 add_test。
include(GoogleTest)
gtest_discover_tests(chatroom_tests)
