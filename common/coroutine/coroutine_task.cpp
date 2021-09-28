/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/coroutine/coroutine_task.h"
#include "common/coroutine/coroutine.h"
#include "common/coroutine/tls.hpp"
#include "common/coroutine/svector.hpp"

// 定义该宏会打开调试日志
//#define M_COROUTINE_TASK_LOG (1)

#ifdef M_COROUTINE_TASK_LOG
#include <stdio.h>
    #define COROUTINE_TASK_LOG printf
#else
    #define COROUTINE_TASK_LOG(...) 
#endif

// max reserve coroutine count in CoroutineTask
#ifndef M_MAX_RESERVE_COROUTINE
#define M_MAX_RESERVE_COROUTINE (500)
#endif

struct new_co_task {
    int co_id;
    coroutine_task_func func;
    void* p;
};

typedef svector<new_co_task*> new_co_task_vector;

new_co_task* create_new_co_task(coroutine_task_func func, void*p);
bool recycle_new_co_task(new_co_task*);
void co_task_func(void*);

bool CoroutineTask::doTask(coroutine_task_func func, void*p) {
    if (Coroutine::curid() != M_MAIN_COROUTINE_ID) {
        COROUTINE_TASK_LOG("need to call doTask in main_coroutine\n");
        return false;
    }

    new_co_task* task = create_new_co_task(func, p);
    if (!task) {
        COROUTINE_TASK_LOG("failed to create new_co_task\n");
        return false;
    }

    Coroutine::resume(task->co_id);
    return true;
}

bool CoroutineTask::resumeTask(int co_id) {
    if (Coroutine::curid() == M_MAIN_COROUTINE_ID) {
        COROUTINE_TASK_LOG("need to call resumeTask in main_coroutine\n");
        return false;
    }

    Coroutine::resume(co_id);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

new_co_task* create_new_co_task(coroutine_task_func func, void*p) {
    new_co_task_vector& task_vec = tlsdata<new_co_task_vector, 0>::data();
    new_co_task* task = 0;
    if (task_vec.empty()) {
        // 创建一个
        task = new new_co_task;
        task->co_id = Coroutine::create(co_task_func, task);
    }
    else {
        task = task_vec.back();
        task_vec.pop_back();
    }
    task->func = func;
    task->p = p;
    return task;
}

void co_task_func(void* p) {
    new_co_task* task = (new_co_task*)p;
    int co_id = task->co_id;
    COROUTINE_TASK_LOG("start new corouitine: %d\n", co_id);

    while (true) {
        if (!task->func) {
            break;
        }

        task->func(task->p);
        if (recycle_new_co_task(task)) {
            COROUTINE_TASK_LOG("yield corouitine: %d\n", co_id);
            Coroutine::yield();
            continue;
        }
        break;
    }

    COROUTINE_TASK_LOG("exit corouitine: %d\n", co_id);

#ifndef M_COROUTINE_TASK_LOG
    co_id++;
#endif
}

bool recycle_new_co_task(new_co_task* task) {
    task->func = nullptr;
    task->p = 0;
    new_co_task_vector& task_vec = tlsdata<new_co_task_vector, 0>::data();
    if (task_vec.size() > M_MAX_RESERVE_COROUTINE) {
        delete task;
        return false;
    }
    else {
        task_vec.push_back(task);
        return true;
    }
}
