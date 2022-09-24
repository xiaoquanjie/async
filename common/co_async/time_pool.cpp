/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/time_pool.h"
#include "common/log.h"
#include <map>
#include <list>
#include <string.h>
#include <assert.h>
#include <memory>
#include <time.h>

namespace co_async {
namespace t_pool {

struct TimeNode {
    uint64_t timerId = 0;
    uint64_t expire = 0;
    std::function<void()> cb;
};

struct PoolInfo {
    bool init = false;
    uint32_t maxIntervalDays = 1;
    void** bucket = nullptr;
    uint32_t bigIter = 0;
    uint32_t smallIter = 0;
    uint64_t allocTimerId = 1;  // 分配的id
    uint64_t begTime = 0;

    ~PoolInfo();
};

typedef std::multimap<uint64_t, TimeNode> TimeNodeMulMap;

// 大桶的精度是1秒
const uint32_t gBigBucket = 24 * 3600;
// 小桶的精度是1/10秒
const uint32_t gSmallBucket = 10;
PoolInfo gPoolInfo;

clock_t getMilClock() {
#ifndef WIN32
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	return GetTickCount();
#endif
}

// 释放
PoolInfo::~PoolInfo() {
    if (!this->bucket) {
        return;
    }

    uint32_t totalBigBucket = gBigBucket * this->maxIntervalDays; 
    for (uint32_t idx = 0; idx < totalBigBucket; ++idx) {
        void** b = (void**)this->bucket[idx];
        for (uint32_t idx2 = 0; idx2 < gSmallBucket; ++idx2) {
            void* b2 = b[idx2];
            if (b2) {
                TimeNodeMulMap* p = (TimeNodeMulMap*)b2;
			    delete (p);
            }
        }
        free(b);
    }

    free(this->bucket);
}

// 计算落在哪个桶
bool calcBucket(uint64_t nowMil, uint32_t interval, uint32_t& bigBucket, uint32_t& smallBucket) {
    if (nowMil < gPoolInfo.begTime) {
        assert(false);
        return false;
    }

    uint64_t intervalMil = nowMil - gPoolInfo.begTime + interval;
    uint64_t intervalSec = intervalMil / 1000;

    // 计算落在哪个大桶
    bigBucket = intervalSec % (gBigBucket * gPoolInfo.maxIntervalDays);
    // 计算落在哪个小桶
    smallBucket = ((intervalMil % 1000) / 100) % 10;

    return true;
}

void setMaxInterval(uint32_t days) {
    if (gPoolInfo.init) {
        return;
    }
    if (days == 0) {
        return;
    }
    if (days > 7) {
        days = 7;
    }

    gPoolInfo.maxIntervalDays = days;
}

// 初始化
void init() {
    if (gPoolInfo.init) {
        return;
    }

    uint32_t totalBigBucket = gBigBucket * gPoolInfo.maxIntervalDays;

    // 初始化大桶, 精度是1秒
    gPoolInfo.bucket = (void**)malloc(sizeof(void*) * totalBigBucket);

    for (uint32_t idx = 0; idx < totalBigBucket; ++idx) {
        // 初始化小桶
        void** small = (void**)malloc(sizeof(void*) * gSmallBucket);
        memset(small, 0, sizeof(void*) * gSmallBucket);
        gPoolInfo.bucket[idx] = small;
    }

    gPoolInfo.begTime = getMilClock();
    gPoolInfo.init = true;
}

uint64_t addTimer(uint32_t interval, std::function<void()>func) {
    init();

    if (interval == 0) {
        return 0;
    }

    uint32_t totalBigBucket = gBigBucket * gPoolInfo.maxIntervalDays;

    if (interval > totalBigBucket * 1000) {
		return 0;
	}

    uint64_t nowMil = getMilClock();
    uint32_t big = 0;
    uint32_t small = 0;

    if (!calcBucket(nowMil, interval, big, small)) {
        return 0;
    }

    void** pb = (void**)gPoolInfo.bucket[big]; 
	TimeNodeMulMap* pmap = (TimeNodeMulMap*)(pb[small]);
	if (!pmap) {
		pmap = new TimeNodeMulMap;
		pb[small] = pmap;
	}

    if (gPoolInfo.allocTimerId == 0xFFFFFFFE) {
		gPoolInfo.allocTimerId = 1;
	}

    uint64_t timerId = (big * 10 + small);
	timerId = timerId << 32;
	timerId += (gPoolInfo.allocTimerId++);

    TimeNode node;
	node.expire = nowMil + interval;
	node.cb = func;
	node.timerId = timerId;
	pmap->insert(std::make_pair(node.expire, node));

    log("add timer bigIter:%d, smallIter:%d, timerId:%llu, sysBigIter:%d, sysSmallIter:%d, expire:%llu, nowMil:%llu\n",
		big, small, timerId, gPoolInfo.bigIter, gPoolInfo.smallIter, node.expire, nowMil);

    return timerId;
}

bool cancelTimer(uint64_t id) {
    uint32_t high32Bit = (id >> 32);
    uint32_t big = high32Bit / 10;
    uint32_t small = high32Bit % 10;

    log("cancel timer bigIter:%d, smallIter:%d, timerId:%llu", big, small, id);

    if (big >= gBigBucket * gPoolInfo.maxIntervalDays) {
        assert(false);
        return false;
    }
    if (small >= 10) {
        assert(false);
        return false;
    }

    void** pb = (void**)gPoolInfo.bucket[big];
    TimeNodeMulMap* pmap = (TimeNodeMulMap*)(pb[small]);
    if (!pmap) {
        assert(false);
        return false;
    }

    for (auto iter = pmap->begin(); iter != pmap->end(); ) {
        if (iter->second.timerId == id) {
            pmap->erase(iter);
            if (pmap->empty()) {
                delete pmap;
                pb[small] = 0;
            }
            return true;
        }
        else {
            ++iter;
        }
    }

    return false;
}

bool update() {
    if (!gPoolInfo.init) {
        return false;
    }

    uint64_t nowMil = getMilClock();
    uint32_t big = 0, small = 0;
    if (!calcBucket(nowMil, 0, big, small)) {
        return false;
    }

    int startIter = gPoolInfo.bigIter * 10 + gPoolInfo.smallIter;
	int endIter = big * 10 + small;

    std::list<TimeNode> nodeList;

    for (; startIter <= endIter; ++startIter) {
		int b = startIter / 10;
		int s = startIter % 10;
		void** pb = (void**)gPoolInfo.bucket[b];
		TimeNodeMulMap* pmap = (TimeNodeMulMap*)(pb[s]);
		if (!pmap) {
			continue;
		}
		for (auto iter = pmap->begin(); iter != pmap->end();) {
			if (iter->first <= nowMil) {
				nodeList.push_back(iter->second);
				pmap->erase(iter++);
			}
			else {
				++iter;
			}
		}
		for (auto iter = nodeList.begin(); iter != nodeList.end(); ++iter) {
			// callback
			(*iter).cb();
		}
		nodeList.clear();
		if (pmap->empty()) {
			delete pmap;
			pb[s] = 0;
		}
	}

    gPoolInfo.bigIter = big;
	gPoolInfo.smallIter = small;
    return true;
}

}
}