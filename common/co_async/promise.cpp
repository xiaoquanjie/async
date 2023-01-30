#include "common/co_async/promise.h"
#include "common/co_async/time_pool.h"
#include "common/coroutine/coroutine.hpp"
#include <assert.h>
#include <unordered_set>
#include <unordered_map>

namespace co_async {

uint32_t g_unique_id = 1;
std::unordered_map<uint64_t, std::shared_ptr<UniqueInfo>> g_unique_id_map;

uint64_t genUniqueId(uint32_t coId) {
    // 混合coid编码，是因为uniqId会发生轮回，避免“串包”
    uint32_t id = g_unique_id++;
    uint64_t uniqId = ((uint64_t)id << 32) | coId;
    return uniqId;
}

void addUniqueId(uint64_t id, std::shared_ptr<UniqueInfo> result) {
    assert(g_unique_id_map.find(id) == g_unique_id_map.end());
    g_unique_id_map[id] = result;
}

bool rmUniqueId(uint64_t id) {
    //printf("rm unique info:%ld\n", id);
    auto iter = g_unique_id_map.find(id);
    if (iter != g_unique_id_map.end()) {
        g_unique_id_map.erase(iter);
        return true;
    }
    return false;
}

std::shared_ptr<UniqueInfo> getByUniqueId(uint64_t id) {
    auto iter = g_unique_id_map.find(id);
    if (iter == g_unique_id_map.end()) {
        return nullptr;
    }

    return iter->second;
}

int64_t addTimer(int interval, std::function<void()> func) {
    return t_pool::addTimer(interval, func);
}

void rmTimer(int64_t timerId) {
    t_pool::cancelTimer(timerId);
}

bool loopPromise(uint32_t curTime) {
    return t_pool::update();
}

void Resolve::operator()(std::shared_ptr<void> res) const {
    if (!rmUniqueId(this->result->uniqueId)) {
        //printf("resolve\n");
        return;
    }

    this->result->resolveRes = res;

    if (this->result->timerId != 0) {
        rmTimer(this->result->timerId);
        Coroutine::resume(this->result->coId);
    }
}

void Reject::operator()(int err, const std::string& msg) const {
    if (!rmUniqueId(this->result->uniqueId)) {
        //printf("reject\n");
        return;
    }

    this->result->rejectRes = std::make_shared<PromiseReject>();
    this->result->rejectRes->err = err;
    this->result->rejectRes->msg = msg;

    if (this->result->timerId != 0) {
        rmTimer(this->result->timerId);
        Coroutine::resume(this->result->coId);
    }
}

std::pair<int, std::shared_ptr<void>> promise(std::function<void(Resolve, Reject)> fn, int timeOut) {
    std::pair<int, std::shared_ptr<void>> ret = std::make_pair(E_ERROR, nullptr);
    
    unsigned int coId = Coroutine::curid();
    if (coId == M_MAIN_COROUTINE_ID) {
        assert(false);
        return ret;
    }

    // 生成一个唯一id
    uint64_t uniqueId = genUniqueId(coId);

    auto result = std::make_shared<PromiseResult>();
    result->coId = coId;
    result->uniqueId = uniqueId;

    Resolve resolve;
    resolve.result= result;
    Reject reject;
    reject.result = result;

    // 添加唯一值进去
    auto infoPtr = std::make_shared<UniqueInfo>();
    infoPtr->resolve = resolve;
    infoPtr->reject = reject;
    addUniqueId(uniqueId, infoPtr);

    // 执行异步
    fn(resolve, reject);

    // 如果立马有了结果，则说明没有异步
    if (!result->resolveRes && !result->rejectRes) {
        // 添加定时器
        int64_t timerId = addTimer(timeOut, [result]() {
            if (!rmUniqueId(result->uniqueId)) {
                //printf("timeout\n");
                return;
            }
            result->timeOutFlag = true;
            Coroutine::resume(result->coId);
        });
        
        result->timerId = timerId;
        // 挂起协程
        Coroutine::yield();
    }

    if (result->timeOutFlag) {
        ret.first = E_TIMEOUT;
    }
    else {
        if (result->resolveRes) {
            ret.first = E_OK;
            ret.second = result->resolveRes;
        }
        else {
            ret.first = E_ERROR;
            ret.second = result->rejectRes;
        }
    }

    return ret;
}

void setTimeout(std::function<void()> fn, int timeOut) {
    addTimer(timeOut, fn);
}

}