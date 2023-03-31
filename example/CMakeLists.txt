cmake_minimum_required (VERSION 3.1)
set(TARGET_NAME "example")
project(${TARGET_NAME})

if(USE_ASYNC_CURL)
    add_definitions(-DUSE_ASYNC_CURL)
endif(USE_ASYNC_CURL)

if(USE_ASYNC_REDIS)
    add_definitions(-DUSE_ASYNC_REDIS)
endif(USE_ASYNC_REDIS)

if(USE_ASYNC_MONGO)
    add_definitions(-DUSE_ASYNC_MONGO)
endif(USE_ASYNC_MONGO)

if(USE_ASYNC_MYSQL)
    add_definitions(-DUSE_ASYNC_MYSQL)
endif(USE_ASYNC_MYSQL)

if(USE_IPC)
    add_definitions(-DUSE_IPC)
endif(USE_IPC)

if(USE_TRANS)
    add_definitions(-DUSE_TRANS)
endif(USE_TRANS)

if(USE_NET)
    add_definitions(-DUSE_NET)
endif(USE_NET)

if(USE_ASYNC_RABBITMQ)
    add_definitions(-DUSE_ASYNC_RABBITMQ)
endif(USE_ASYNC_RABBITMQ)

if(USE_ASYNC_ZOOKEEPER)
    add_definitions(-DUSE_ASYNC_ZOOKEEPER)
endif(USE_ASYNC_ZOOKEEPER)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
file(GLOB_RECURSE ALL_CPP *.cpp *.cc)

include_directories(./)
include_directories(../)

link_directories(/usr/local/lib64 /usr/local/lib)

add_executable(${TARGET_NAME} ${ALL_CPP})

add_dependencies(${TARGET_NAME} async)

target_link_libraries(${TARGET_NAME} PRIVATE async pthread)
