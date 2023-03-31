cmake_minimum_required (VERSION 3.1)

set(TARGET_NAME "serve")

project(${TARGET_NAME} CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#file(GLOB_RECURSE ALL_CPP *.cpp *.c)
# 不使用file(GLOB_RECURSE ALL_CPP *.cpp *.c),因为它会自动创建CMakeCxxCompilerId.cpp，尚未找到答案
aux_source_directory(. ALL_CPP)

include_directories(../../)
link_directories(/usr/local/lib64 /usr/local/lib)

if(USE_IPC)
    add_definitions(-DUSE_IPC)
endif(USE_IPC)

if(USE_NET)
    add_definitions(-DUSE_NET)
endif(USE_NET)

set(LINK_LIBRARYS ${LINK_LIBRARYS} async)

add_library(${TARGET_NAME} SHARED ${ALL_CPP})

add_dependencies(${TARGET_NAME} async)

# 指定链接库
target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARYS})
