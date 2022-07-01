#include "common/co_async/promise.h"
#include "common/co_async/time_pool.h"
#include "common/coroutine/coroutine.hpp"
#include <assert.h>
#include <unordered_set>

namespace co_async {

TimerPool g_time_pool;
int64_t g_unique_id = 1;
std::unordered_set<int64_t> g_unique_id_set;

int64_t genUniqueId() {
    return g_unique_id++;
}

void addUniqueId(int64_t id) {
    assert(g_unique_id_set.find(id) == g_unique_id_set.end());
    g_unique_id_set.insert(id);
}

bool rmUniqueId(int64_t id) {
    auto iter = g_unique_id_set.find(id);
    if (iter != g_unique_id_set.end()) {
        g_unique_id_set.erase(iter);
        return true;
    }
    return false;
}

int64_t addTimer(int interval, std::function<void()> func) {
    return g_time_pool.AddTimer(interval, func);
}

void rmTimer(int64_t timer_id) {
    g_time_pool.CancelTimer(timer_id);
}

bool loop(uint32_t cur_time) {
    g_time_pool.Update();
    return true;
}

int wait(uint32_t interval) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_ERROR;
    }

    addTimer(interval, [co_id]() {
        Coroutine::resume(co_id);
    });

    Coroutine::yield();
    return E_OK;
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
    int64_t uniqueId = genUniqueId();

    // 添加唯一值进去
    addUniqueId(uniqueId);

    auto result = std::make_shared<PromiseResult>();
    result->coId = coId;
    result->uniqueId = uniqueId;

    Resolve resolve;
    resolve.result= result;
    Reject reject;
    reject.result = result;

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