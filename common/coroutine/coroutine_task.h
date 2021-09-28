/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

typedef std::function<void(void*)> coroutine_task_func;

class CoroutineTask {
public:
    static bool doTask(coroutine_task_func func, void*p);

    static bool resumeTask(int co_id);
};

