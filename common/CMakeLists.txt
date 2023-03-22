cmake_minimum_required (VERSION 3.1)
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
    if(USE_ASYNC_MYSQL)
        #link_directories(/usr/local/lib/mariadb)
        link_directories(/usr/lib64/mysql)
    endif(USE_ASYNC_MYSQL)
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
    set(LINK_LIBRARYS ${LINK_LIBRARYS} event_core hiredis)
endif(USE_ASYNC_REDIS)

if(USE_ASYNC_CLUSTER_REDIS)
    add_definitions(-DUSE_ASYNC_CLUSTER_REDIS)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} hiredis_cluster)
endif(USE_ASYNC_CLUSTER_REDIS)

if(USE_ASYNC_MONGO)
    add_definitions(-DUSE_ASYNC_MONGO)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} bson-1.0 mongoc-1.0)
endif(USE_ASYNC_MONGO)

if(USE_ASYNC_MYSQL)
    add_definitions(-DUSE_ASYNC_MYSQL)
    #set(LINK_LIBRARYS ${LINK_LIBRARYS} mariadb)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} mysqlclient)
endif(USE_ASYNC_MYSQL)

if(USE_IPC)
    add_definitions(-DUSE_IPC)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} zmq)
endif(USE_IPC)

if(USE_TRANS)
    add_definitions(-DUSE_TRANS)
endif(USE_TRANS)

if(USE_NET)
    add_definitions(-DUSE_NET)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} event_core event_extra)
endif(USE_NET)

if(USE_ASYNC_RABBITMQ)
    add_definitions(-DUSE_ASYNC_RABBITMQ)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} rabbitmq)
endif(USE_ASYNC_RABBITMQ)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

if(TRACE)
    add_definitions(-DTRACE)
endif(TRACE)

target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARYS})