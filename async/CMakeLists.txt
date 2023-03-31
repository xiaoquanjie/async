cmake_minimum_required (VERSION 3.1)
set(TARGET_NAME "async")
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

    file(COPY ./async/curl/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/curl/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )

    file(COPY ./co_async/curl/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/curl/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_CURL)

if(USE_ASYNC_REDIS)
    add_definitions(-DUSE_ASYNC_REDIS)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} event_core hiredis)

    file(COPY ./async/redis/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/redis/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
    file(COPY ./co_async/redis/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/redis/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_REDIS)

if(USE_ASYNC_CLUSTER_REDIS)
    add_definitions(-DUSE_ASYNC_CLUSTER_REDIS)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} hiredis_cluster)
endif(USE_ASYNC_CLUSTER_REDIS)

if(USE_ASYNC_MONGO)
    add_definitions(-DUSE_ASYNC_MONGO)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} bson-1.0 mongoc-1.0)

    file(COPY ./async/mongo/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/mongo/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )

    file(COPY ./co_async/mongo/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/mongo/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_MONGO)

if(USE_ASYNC_MYSQL)
    add_definitions(-DUSE_ASYNC_MYSQL)
    #set(LINK_LIBRARYS ${LINK_LIBRARYS} mariadb)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} mysqlclient)

    file(COPY ./async/mysql/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/mysql/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
    file(COPY ./co_async/mysql/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/mysql/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_MYSQL)

if(USE_ASYNC_RABBITMQ)
    add_definitions(-DUSE_ASYNC_RABBITMQ)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} rabbitmq)

    file(COPY ./async/rabbitmq/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/rabbitmq/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
    file(COPY ./co_async/rabbitmq/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/coasync/rabbitmq/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_RABBITMQ)

if(USE_ASYNC_ZOOKEEPER)
    add_definitions(-DUSE_ASYNC_ZOOKEEPER)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} zookeeper_st)

    file(COPY ./async/zookeeper/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/zookeeper/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
    file(COPY ./co_async/zookeeper/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/zookeeper/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_ASYNC_ZOOKEEPER)

if(USE_IPC)
    add_definitions(-DUSE_IPC)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} zmq)

    file(COPY ./async/ipc/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/ipc/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )    
endif(USE_IPC)

if(USE_TRANS)
    add_definitions(-DUSE_TRANS)

    file(COPY ./async/transaction/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/transaction/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif(USE_TRANS)

if(USE_NET)
    add_definitions(-DUSE_NET)
    set(LINK_LIBRARYS ${LINK_LIBRARYS} event_core event_extra)

    file(COPY ./async/net/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/net/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
    file(COPY ./async/signal/
        DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/signal/
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )       
endif(USE_NET)

file(COPY ./async/
    DESTINATION ${CMAKE_BINARY_DIR}/include/async/async/
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

file(COPY ./co_async/
    DESTINATION ${CMAKE_BINARY_DIR}/include/async/co_async/
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

file(COPY ./coroutine/
    DESTINATION ${CMAKE_BINARY_DIR}/include/async/coroutine/
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

file(COPY ./threads/
    DESTINATION ${CMAKE_BINARY_DIR}/include/async/threads/
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

file(COPY ./
    DESTINATION ${CMAKE_BINARY_DIR}/include/async/
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

if(TRACE)
    add_definitions(-DTRACE)
endif(TRACE)

target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARYS})