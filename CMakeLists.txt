cmake_minimum_required (VERSION 2.8)
set(TARGET_NAME "test")
project(${TARGET_NAME})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(USE_ASYNC_CURL)
    add_definitions(-DUSE_ASYNC_CURL)
endif(USE_ASYNC_CURL)

if(USE_ASYNC_REDIS)
    add_definitions(-DUSE_ASYNC_REDIS)
endif(USE_ASYNC_REDIS)

if(USE_ASYNC_MONGO)
    add_definitions(-DUSE_ASYNC_MONGO)
endif(USE_ASYNC_MONGO)

set(PROJECT_LIB_TARGETS "common")

add_subdirectory(common)
add_subdirectory(example)
