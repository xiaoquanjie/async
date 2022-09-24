/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <memory>

namespace co_async {
namespace t_pool {

// �������ļ������, ����ܳ���7�죬Ĭ����1��
// �����Ҫ��addTimer֮ǰ�͵���
void setMaxInterval(uint32_t days);

uint64_t addTimer(uint32_t interval, std::function<void()>func);

bool cancelTimer(uint64_t id);

// ���ô˺�������ʱ�Ľڵ�ᱻ���ûص���ʱ���Ǻ���
bool update();

}
}