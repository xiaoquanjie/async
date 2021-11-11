/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include <map>
#include <list>
#include <string.h>
#include <assert.h>
#include "common/co_bridge/time_pool.h"

#ifndef WIN32
#else
#endif

namespace co_bridge {

#define M_TIME_POOL_PRINTF(...)
//#define M_TIME_POOL_PRINTF printf

// 大桶的精度是1秒
static const int big_bucket_cnt = 24 * 3600;
// 小桶的精度是1/10秒
static const int small_bucket_cnt = 10;

typedef std::multimap<unsigned long long, TimerPool::TimeNode*> TimeNodeMap;

clock_t get_mil_clock() {
#ifndef WIN32
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	return GetTickCount();
#endif
}

TimerPool::TimerPool() {
}

TimerPool::~TimerPool() {
	for (int idx = 0; idx < big_bucket_cnt * _max_interval_day; ++idx) {
		for (int idx2 = 0; idx2 < small_bucket_cnt; ++idx2) {
			void** pp = (void**)_bucket[idx];
			TimeNodeMap* p = (TimeNodeMap*)pp[idx2];
			delete (p);
		}
		free(_bucket[idx]);
	}
	free(_bucket);
}

void TimerPool::Init(int max_interval_day) {
	if (_init) {
		return;
	}

	_init = true;
	if (max_interval_day > 7 || max_interval_day < 0) {
		max_interval_day = 1;
	}
	_max_interval_day = max_interval_day;

	// 初始化大桶, 精度是1秒
	_bucket = (void**)malloc(sizeof(void*) * big_bucket_cnt * _max_interval_day);
	for (int idx = 0; idx < big_bucket_cnt * _max_interval_day; ++idx) {
		// 初始化小桶
		void** small_bucket_pointer = (void**)malloc(sizeof(void*) * small_bucket_cnt);
		memset(small_bucket_pointer, 0, sizeof(void*) * small_bucket_cnt);
		_bucket[idx] = (TimeNodeMap*)small_bucket_pointer;
	}

	_beg_time = get_mil_clock();
	_big_bucket = 0;
	_small_bucket = 0;
	_cur_timer_id = 1;
}

void TimerPool::Update() {
	Init();

	int big_bucket = 0;
	int small_bucket = 0;
	unsigned long long now = get_mil_clock();
	_CalcBucket(now, 0, big_bucket, small_bucket);

	if (big_bucket < 0 || small_bucket < 0) {
		return;
	}

	// 桶轮循
	std::list<TimeNode*> nodelist;
	int s_idx = _big_bucket * 10 + _small_bucket;
	int e_idx = big_bucket * 10 + small_bucket;
	for (; s_idx <= e_idx; ++s_idx) {
		int b = s_idx / 10;
		int s = s_idx % 10;
		void** pp = (void**)_bucket[b];
		TimeNodeMap* pmap = (TimeNodeMap*)(pp[s]);
		if (!pmap) {
			continue;
		}
		for (auto iter = pmap->begin(); iter != pmap->end();) {
			if (iter->first <= now) {
				nodelist.push_back(iter->second);
				pmap->erase(iter++);
			}
			else {
				++iter;
			}
		}
		for (auto iter = nodelist.begin(); iter != nodelist.end(); ++iter) {
			// callback
			(*iter)->cb();
			// recycle memory
			delete (*iter);
		}
		nodelist.clear();
		if (pmap->empty()) {
			delete pmap;
			pp[s] = 0;
		}
	}
	_big_bucket = big_bucket;
	_small_bucket = small_bucket;
}

unsigned long long TimerPool::AddTimer(int interval, std::function<void()>func) {
	Init();

	if (interval <= 0) {
		return 0;
	}
	if (interval > big_bucket_cnt * _max_interval_day * 1000) {
		return 0;
	}

	unsigned long long now_mil = get_mil_clock();

	int big_bucket = 0;
	int small_bucket = 0;
	_CalcBucket(now_mil, interval, big_bucket, small_bucket);

	if (big_bucket < 0 || small_bucket < 0) {
		return 0;
	}

	void** pp = (void**)_bucket[big_bucket];
	TimeNodeMap* pmap = (TimeNodeMap*)(pp[small_bucket]);
	if (!pmap) {
		pmap = new TimeNodeMap;
		pp[small_bucket] = pmap;
	}
	if (_cur_timer_id == 0xFFFFFFFE) {
		_cur_timer_id = 1;
	}

	unsigned long long timer_id = (big_bucket * 10 + small_bucket);
	timer_id = timer_id << 32;
	timer_id += _cur_timer_id++;

	TimeNode* node = new TimeNode;
	node->expire = now_mil + interval;
	node->cb = func;
	node->timer_id = timer_id;
	pmap->insert(std::make_pair(node->expire, node));
	M_TIME_POOL_PRINTF("add timer big_bucket:%d, small_bucket:%d, timer_id:%llu, sys_big_bucket:%d, sys_small_bucket:%d, expire:%llu, now_mil:%llu\n",
		big_bucket, small_bucket, timer_id, _big_bucket, _small_bucket, node->expire, now_mil);
	return timer_id;
}

int TimerPool::CancelTimer(unsigned long long id) {
	unsigned long long flag = 0xFFFFFFFF;
	/*int low_32bit = (int)(id & flag);*/
	int high_32bit = (id >> 32) & flag;

	int big_bucket = high_32bit / 10;
	int small_bucket = high_32bit % 10;

	M_TIME_POOL_PRINTF("cancel timer big_bucket:%d, small_bucket:%d, timer_id:%llu, sys_big_bucket:%d, sys_small_bucket:%d\n", 
		big_bucket, small_bucket, id, _big_bucket, _small_bucket);
	void** pp = (void**)_bucket[big_bucket];
	TimeNodeMap* pmap = (TimeNodeMap*)(pp[small_bucket]);
	if (!pmap) {
		printf("cancel timer big_bucket:%d, small_bucket:%d, timer_id:%llu, sys_big_bucket:%d, sys_small_bucket:%d\n", 
			big_bucket, small_bucket, id, _big_bucket, _small_bucket);
		assert(false);
		return -1;
	}
	for (auto iter = pmap->begin(); iter != pmap->end();) {
		if (iter->second->timer_id == id) {
			delete iter->second;
			pmap->erase(iter);
			if (pmap->empty()) {
				delete pmap;
				pp[small_bucket] = 0;
			}
			return 0;
		}
		else {
			++iter;
		}
	}
	//assert(false);
	return -1;
}

bool TimerPool::_CalcBucket(unsigned long long now_mil, int interval, int& big_bucket, int& small_bucket) {
	unsigned long long inteval_mil = (now_mil - _beg_time + interval);
	//assert(inteval_mil > 0);
	int interval_sec = inteval_mil / 1000;

	big_bucket = interval_sec % (big_bucket_cnt * _max_interval_day);
	small_bucket = ((inteval_mil % 1000) / 100) % 10;

	if (big_bucket < 0 || small_bucket < 0 || now_mil < _beg_time) {
		M_TIME_POOL_PRINTF("now_mil:%llu, _beg_time:%llu\n", now_mil, _beg_time);
		assert(false);
	}
	return true;
}

}