cmake_minimum_required(VERSION 3.16)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

include_directories(include)

add_executable(
    chatroom src/main_server.cpp
)

enable_testing()

find_package(GTest REQUIRED)

add_executable(
    chatroom_tests tests/test_main.cpp
)

target_include_directories(
    chatroom_tests
    PRIVATE include
)

target_link_libraries(
    chatroom_tests
    PRIVATE
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(chatroom_tests)
