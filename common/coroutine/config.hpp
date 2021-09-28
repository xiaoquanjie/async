/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#ifndef M_PLATFORM_WIN32
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define M_PLATFORM_WIN32 1
    #endif
#endif

#ifndef M_PLATFORM_WIN
    #if defined(M_PLATFORM_WIN32) || defined(WIN64)
        #define M_PLATFORM_WIN 1
    #endif
#endif

#ifdef M_PLATFORM_WIN
    #ifndef M_WIN32_LEAN_AND_MEAN  
        #define WIN32_LEAN_AND_MEAN
    #endif // end M_WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    #include <pthread.h>
    #include <ucontext.h>
#endif // end M_PLATFORM_WIN

// common header
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#ifndef COROUTINE_READY
#define COROUTINE_READY   (1)
#endif

#ifndef COROUTINE_RUNNING
#define COROUTINE_RUNNING (2)
#endif

#ifndef COROUTINE_SUSPEND
#define COROUTINE_SUSPEND (3)
#endif

#ifndef COROUTINE_DEAD
#define COROUTINE_DEAD	  (4)
#endif

#ifndef DEFAULT_COROUTINE
#define DEFAULT_COROUTINE (1024)
#endif

// invalid coroutine id
#ifndef M_INVALID_COROUTINE_ID
#define M_INVALID_COROUTINE_ID (-1)
#endif

// main coroutine id
#ifndef M_MAIN_COROUTINE_ID
#define M_MAIN_COROUTINE_ID (0)
#endif

// for private stack
#ifndef M_COROUTINE_STACK_SIZE
#define M_COROUTINE_STACK_SIZE  4*1024*1024
#endif

// coroutine func type
typedef void(*_coroutine_func_)(void*ud);

