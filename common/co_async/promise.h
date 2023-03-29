/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * 
*/

#pragma once

#include <functional>
#include "common/co_async/comm.hpp"

namespace co_async {

// 循环，内部调用异步循环
bool loopPromise(uint32_t curTime = 0);

// 返回值是pair: first=[E_OK/E_TIMEOUT/E_ERROR], second=[std::shared_ptr<void>/nullptr/other]
// 1、first=E_OK时, second=目标
// 2、first=E_TIMEOUT时，second=nullptr
// 3、first=E_ERROR时，second=[std::shared_ptr<PromiseReject>/nullptr]
std::pair<int, std::shared_ptr<void>> promise(std::function<void(Resolve, Reject)> fn, int timeOut);

void setTimeout(std::function<void()> fn, int timeOut);

// 协程等待n毫秒
void wait(int timeOut);

}



