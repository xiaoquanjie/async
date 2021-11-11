/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <time.h>
#include <functional>

namespace co_bridge {

class TimerPool {
public:
	struct TimeNode {
		unsigned long long timer_id;
		unsigned long long expire;
		std::function<void()> cb;
	};

	// @max_interval_day 最大不能超过7天，默认是7天
	TimerPool();

	~TimerPool();

	void Init(int max_interval_day = 2);

	// 调用此函数，超时的节点会被调用回调，时间是毫秒
	void Update();

	// @interval是毫秒, 返回值是timer id, 值大于0
	unsigned long long AddTimer(int interval, std::function<void()>func);

	int CancelTimer(unsigned long long id);

protected:
	bool _CalcBucket(unsigned long long now_mil, int interval, int& big_bucket, int& small_bucket);

private:
	bool _init = false;
	int _max_interval_day = 0;	
	void** _bucket = 0;			
	unsigned long long _beg_time = 0;
	int _big_bucket = 0;
	int _small_bucket = 0;
	unsigned long long _cur_timer_id = 1;
};

}