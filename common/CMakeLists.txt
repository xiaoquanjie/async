cmake_minimum_required (VERSION 2.8)
set(TARGET_NAME "common")
project(${TARGET_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
file(GLOB_RECURSE ALL_CPP *.cpp)

include_directories(../)

if(WIN32)
    include_directories(${WIN32_ROOT_INCLUDE_DIR}/libbson-1.0)
    include_directories(${WIN32_ROOT_INCLUDE_DIR}/libmongoc-1.0)
else(WIN32)
    include_directories(/usr/local/include/libmongoc-1.0)
	include_directories(/usr/local/include/libbson-1.0)
endif(WIN32)

link_directories(/usr/local/lib64 /usr/local/lib)

add_library(${TARGET_NAME} SHARED ${ALL_CPP})

target_link_libraries(${TARGET_NAME} pthread event_core hiredis_vip hiredis curl bson-1.0 mongoc-1.0)