/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "common/transaction/transaction_mgr.h"
#include "common/coroutine/coroutine.hpp"
#include <unordered_map>
#include <assert.h>
#include <stdarg.h>

namespace trans_mgr {

/////////////////////////////////////////////////////////////////////////////////

uint32_t g_trans_id = 1;
uint32_t g_max_concurrent_trans = 100000;
uint32_t g_cur_concurrent_trans = 0;
time_t   g_last_check_time = 0;
std::unordered_map<uint32_t, TransactionBucket*> g_trans_bucket_map;
std::unordered_map<uint64_t, std::shared_ptr<void>> g_context_map;
std::list<BaseTickTransaction*> g_tick_trans_list;

////////////////////////////////////////////////////////////////////////////////

// 日志输出接口
std::function<void(const char*)> g_log_cb = [](const char* data) {
    printf("[transaction] %s\n", data);
};

void log(const char* format, ...) {
    if (!g_log_cb) {
        return;
    }

    char buf[1024] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    g_log_cb(buf);
}

void setLogFunc(std::function<void(const char*)> cb) {
    g_log_cb = cb;
}

uint32_t genTransId() {
    return (g_trans_id++);
}

void setMaxTrans(uint32_t max_trans) {
    g_max_concurrent_trans = max_trans;
}

void setTransContext(std::shared_ptr<void> ctx) {
    auto co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return;
    }

    g_context_map[co_id] = ctx;
}

std::shared_ptr<void> getTransContext() {
    auto co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return 0;
    }
    auto iter = g_context_map.find(co_id);
    if (iter == g_context_map.end()) {
        return 0;
    }
    return iter->second;
}

void clearTransContext() {
    auto co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return;
    }

    g_context_map.erase(co_id);
}

int handle(uint32_t req_cmd_id, const char* packet, uint32_t packet_size, void* ext) {
    if (g_cur_concurrent_trans >= g_max_concurrent_trans) {
        log("over max concurrent trans limit:%d\n", g_cur_concurrent_trans);
        return -1;
    }

    auto iter = g_trans_bucket_map.find(req_cmd_id);
    if (iter == g_trans_bucket_map.end()) {
        return -2;
    }

    BaseTransaction* t = iter->second->Create();
    if (!t) {
        assert(false);
        return -1;
    }

    g_cur_concurrent_trans++;
    t->Handle(packet, packet_size, ext);
    return 0;
}

void tick(uint32_t cur_time) {
    for (auto iter = g_tick_trans_list.begin(); iter != g_tick_trans_list.end(); ++iter) {
        (*iter)->Tick(cur_time);
    }
    if (cur_time - g_last_check_time >= 120) {
        g_last_check_time = cur_time;
        log("[trans statistics] cur_trans:%d\n", g_cur_concurrent_trans);
    }
}

int registBucket(TransactionBucket* bucket) {
    auto iter = g_trans_bucket_map.find(bucket->ReqCmdId());
    if (iter != g_trans_bucket_map.end()) {
        delete bucket;
        assert(false);
        return -1;
    }

    log("regist req_cmd:%d, rsp_cmd:%d, trans:%s\n", bucket->ReqCmdId(), bucket->RspCmdId(), bucket->TransName());
    g_trans_bucket_map.insert(std::make_pair(bucket->ReqCmdId(), bucket));
    return 0;
}

void recycleTransaction(BaseTransaction* t) {
    if (t->ReqCmdId() == 0 && t->RspCmdId() == 0) {
        return;
    }
    auto iter = g_trans_bucket_map.find(t->ReqCmdId());
    if (iter == g_trans_bucket_map.end()) {
        assert(false);
        return;          
    }

    g_cur_concurrent_trans--;
    iter->second->Recycle(t);
}

int registTickTransaction(TransactionBucket* bucket) {
    BaseTransaction* t = bucket->Create();
    BaseTickTransaction* tt = dynamic_cast<BaseTickTransaction*>(t);
    g_tick_trans_list.push_back(tt);
    delete bucket;
    return 0;
}

}

#endif