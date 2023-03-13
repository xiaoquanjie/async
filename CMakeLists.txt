cmake_minimum_required (VERSION 3.1)
set(TARGET_NAME "test")
project(${TARGET_NAME})

# 添加开启功能的宏
#set(USE_ASYNC_CURL 1)
#set(USE_ASYNC_REDIS 1)
#set(USE_ASYNC_MONGO 1)
#set(USE_ASYNC_MYSQL 1)
#set(USE_IPC 1)
#set(USE_NET 1)
#set(USE_TRANS 1)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROJECT_LIB_TARGETS "common")

add_subdirectory(common)
add_subdirectory(example)
add_subdirectory(serve)
