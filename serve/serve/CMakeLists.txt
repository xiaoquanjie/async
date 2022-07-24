cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "serve")
set(TARGET_EXE_NAME "testServe")

project(${TARGET_NAME} CXX)
project(${TARGET_EXE_NAME} CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

#file(GLOB_RECURSE ALL_CPP *.cpp *.c)
# 不使用file(GLOB_RECURSE ALL_CPP *.cpp *.c),因为它会自动创建CMakeCxxCompilerId.cpp，尚未找到答案
aux_source_directory(. ALL_CPP)

include_directories(../../)

if(USE_IPC)
    add_definitions(-DUSE_IPC)
endif(USE_IPC)

set(LINK_LIBRARYS ${LINK_LIBRARYS} common)

add_library(${TARGET_NAME} SHARED ${ALL_CPP})

add_definitions(-DTEST_MAIN)
add_executable(${TARGET_EXE_NAME} ${ALL_CPP})

add_dependencies(${TARGET_NAME} "common")
add_dependencies(${TARGET_EXE_NAME} "common")

# 指定链接库
target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARYS})
target_link_libraries(${TARGET_EXE_NAME} PRIVATE ${LINK_LIBRARYS})