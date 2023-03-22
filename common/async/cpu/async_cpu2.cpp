/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "common/async/cpu/async_cpu.h"

#if CUR_CPU_VERSION == 2

#include "common/tls.hpp"
#include "common/log.h"
#include <queue>
#include <mutex>
#include <assert.h>
#include <memory>

namespace async {
namespace cpu {

// 请求数据
struct CpuReqData {
    async_cpu_op op;
    async_cpu_cb cb;
    void *userData = 0;
};

typedef std::shared_ptr<CpuReqData> CpuReqDataPtr;

// 回复数据
struct CpuRspData {
    CpuReqDataPtr req;
    // 运算回复结果
    int64_t result = 0;
};

typedef std::shared_ptr<CpuRspData> CpuRspDataPtr;

// 运行时数据
struct CpuThreadData {
    // 是否初始化
    bool isInit = false;
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::queue<CpuReqDataPtr> reqQueue;
    std::queue<CpuRspDataPtr> rspQueue;
    // 回复队列锁
    std::mutex rspQueueMutex;
    // 上一次统计输入时间
    time_t lastPrintTime = 0;
};

CpuThreadData* threadData() {
    CpuThreadData& data = async::tlsdata<CpuThreadData>::data();
    return &data;
}

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;

#define cpuLog(format, ...) log("[async_cpu] " format, __VA_ARGS__)

void threadProcess(CpuReqDataPtr req, CpuThreadData* data) {
    // 执行结果
    int64_t result = req->op(req->userData);

    auto rsp = std::make_shared<CpuRspData>();
    rsp->result = result;
    rsp->req = req;

    // 将结果存放回队列，需要加锁
    data->rspQueueMutex.lock();
    data->rspQueue.emplace(rsp);
    data->rspQueueMutex.unlock();
}

// 取一条请求，把该请求包装成function，以便其他线程可以运行它
// 以函数都是调用线程在pop，所以不会有竞争问题，不需要锁
std::function<void()> localPop(CpuThreadData* data) {
    if (data->reqQueue.empty()) {
        assert(false);
        return std::function<void()>();
    }

    auto req = data->reqQueue.front();
    data->reqQueue.pop();

    auto f = std::bind(threadProcess, req, data);
    return f;
}

// 取结果
void localRespond(CpuThreadData* data) {
    if (data->rspQueue.empty()) {
        return;
    }

    // 当帧一次性所全部结果取出来
    std::queue<CpuRspDataPtr> que;
    // 加锁
    data->rspQueueMutex.lock();
    // 交换队列
    que.swap(data->rspQueue);
    data->rspQueueMutex.unlock();

    // 回调结果
    while (!que.empty()) {
        data->rspTaskCnt++;
        CpuRspDataPtr rsp = que.front();
        que.pop();

        rsp->req->cb(rsp->result, rsp->req->userData);
    }
}

// 取出一个任务交给io线程
void localProcess(CpuThreadData* data) {
    if (data->reqQueue.empty()) {
        return;
    }

    if (gIoThr) {
        gIoThr(localPop(data));
    } else {
        // 调用线程担任io线程的角色
        localPop(data)();
    }
}

// 统计
void localStatistics(CpuThreadData* data, uint32_t curTime) {
    if (curTime - data->lastPrintTime < 120) {
        return;
    }

    data->lastPrintTime = curTime;
    cpuLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        data->reqTaskCnt - data->rspTaskCnt,
        data->reqTaskCnt,
        data->rspTaskCnt);
}

//////////////////////////////////////////// public //////////////////////////////

void execute(async_cpu_op op, void* userData, async_cpu_cb cb) {
    CpuThreadData* data = threadData();
    if (!data->isInit) {
        data->isInit = true;
    }

    auto req = std::make_shared<CpuReqData>();
    req->op = op;
    req->cb = cb;
    req->userData = userData;

    // 放入请求队列
    data->reqQueue.emplace(req);
    data->reqTaskCnt++;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

bool loop(uint32_t curTime) {
    CpuThreadData* data = threadData();
    if (!data->isInit) {
        return false;
    }

    localRespond(data);
    localProcess(data);
    localStatistics(data, curTime);

    // 是否有任务, 数目不相等，说明有结果在等待。因为不会出现reqTaskCnt不等于rspTaskCnt的错误情况
    bool hasTask = data->reqTaskCnt != data->rspTaskCnt;
    return hasTask;
}


} // cpu
} // async

#endif