cmake_minimum_required (VERSION 2.8)
set(TARGET_NAME "test")
project(${TARGET_NAME})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(USE_ASYNC_MYSQL)
    add_definitions(-DUSE_ASYNC_MYSQL)
endif(USE_ASYNC_MYSQL)

set(PROJECT_LIB_TARGETS "common")

add_subdirectory(common)
add_subdirectory(example)
