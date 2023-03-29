/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <memory>

namespace co_async {
namespace t_pool {

// 设置最大的间隔小时, 最大不能超过24，默认是1小时
// 这个需要在addTimer之前就调用
void setMaxInterval(uint32_t hours);

// 不允许interval的值超过setMaxInterval的限制
uint64_t addTimer(uint32_t interval, std::function<void()>func);

bool cancelTimer(uint64_t id);

// 调用此函数，超时的节点会被调用回调，时间是毫秒
bool update();

}
}