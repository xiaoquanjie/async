cmake_minimum_required (VERSION 2.8)
set(TARGET_NAME "example")
project(${TARGET_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
file(GLOB_RECURSE ALL_CPP *.cpp)

include_directories(./)
include_directories(../)

add_executable(${TARGET_NAME} ${ALL_CPP})

add_dependencies(${TARGET_NAME} ${PROJECT_LIB_TARGETS})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_LIB_TARGETS})
