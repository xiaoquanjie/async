cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "testServe")

project(${TARGET_NAME} CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#file(GLOB_RECURSE ALL_CPP *.cpp *.c)
# 不使用file(GLOB_RECURSE ALL_CPP *.cpp *.c),因为它会自动创建CMakeCxxCompilerId.cpp，尚未找到答案
aux_source_directory(. ALL_CPP)

include_directories(../../)

set(LINK_LIBRARYS ${LINK_LIBRARYS} serve common)

add_executable(${TARGET_NAME} ${ALL_CPP})

# 指定链接库
target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARYS})