cmake_minimum_required (VERSION 2.8)
set(TARGET_NAME "common")
project(${TARGET_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Wno-error=deprecated-declarations -Wno-deprecated-declarations")
file(GLOB_RECURSE ALL_CPP *.cpp)

include_directories(../)

if(WIN32)
    if(USE_ASYNC_MONGO)
        include_directories(${WIN32_ROOT_INCLUDE_DIR}/libbson-1.0)
        include_directories(${WIN32_ROOT_INCLUDE_DIR}/libmongoc-1.0)
    endif(USE_ASYNC_MONGO)
else(WIN32)
    if(USE_ASYNC_MONGO)
        include_directories(/usr/local/include/libmongoc-1.0)
	    include_directories(/usr/local/include/libbson-1.0)
    endif(USE_ASYNC_MONGO)
endif(WIN32)

link_directories(/usr/local/lib64 /usr/local/lib)

add_library(${TARGET_NAME} SHARED ${ALL_CPP})

set(LINK_LIBRARYS pthread)

if(USE_ASYNC_CURL)
    add_definitions(-DUSE_ASYNC_CURL)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} curl)
endif(USE_ASYNC_CURL)

if(USE_ASYNC_REDIS)
    add_definitions(-DUSE_ASYNC_REDIS)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} event_core hiredis_vip hiredis)
endif(USE_ASYNC_REDIS)

if(USE_ASYNC_MONGO)
    add_definitions(-DUSE_ASYNC_MONGO)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} bson-1.0 mongoc-1.0)
endif(USE_ASYNC_MONGO)

target_link_libraries(${TARGET_NAME} ${LINK_LIBRARYS})