# 统一的编译警告集合。
#
# 为什么单独成文件：多个 target 共享同一套规则，改一处全体生效；
# 也避免顶层 CMakeLists 被一堆 -W 淹没。

add_library(chatroom_warnings INTERFACE)

target_compile_options(chatroom_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    # 网络代码里 ssize_t / size_t / int 之间来回转换是 bug 温床，
    # 这两条强迫你把每一次转换写出来。
    -Wconversion
    -Wsign-conversion
)

if(CHATROOM_WERROR)
    target_compile_options(chatroom_warnings INTERFACE -Werror)
endif()
