/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "async/transaction/transaction_mgr.h"
#include "async/coroutine/coroutine.hpp"
#include "async/log.h"
#include <unordered_map>
#include <assert.h>
#include <algorithm>

namespace trans_mgr {

/////////////////////////////////////////////////////////////////////////////////

uint32_t g_trans_id = 1;
uint32_t g_max_concurrent_trans = 100000;
uint32_t g_cur_concurrent_trans = 0;
time_t   g_last_check_time = 0;
std::unordered_map<uint32_t, TransactionBucket*> g_trans_bucket_map;
std::unordered_map<uint32_t, std::unordered_map<std::string, TransactionBucket*>> g_http_trans_bucket_map;
std::unordered_map<uint64_t, std::shared_ptr<void>> g_context_map;
std::list<BaseTickTransaction*> g_tick_trans_list;

////////////////////////////////////////////////////////////////////////////////

uint32_t genTransId() {
    return (g_trans_id++);
}

void setMaxTrans(uint32_t max_trans) {
    g_max_concurrent_trans = max_trans;
}

void setTransCxt(std::shared_ptr<void> ctx) {
    auto co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return;
    }

    g_context_map[co_id] = ctx;
}

std::shared_ptr<void> getTransCxt() {
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

void clearTransCxt() {
    auto co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return;
    }

    g_context_map.erase(co_id);
}

int handle(uint32_t req_cmd_id, const char* packet, uint32_t packet_size, void* ext) {
    if (g_cur_concurrent_trans >= g_max_concurrent_trans) {
        log("[transaction] over max concurrent trans limit:%d", g_cur_concurrent_trans);
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

int handle(uint32_t id, std::string url, const char* packet, uint32_t packet_size, void* ext) {
    if (g_cur_concurrent_trans >= g_max_concurrent_trans) {
        log("[transaction] over max concurrent trans limit:%d", g_cur_concurrent_trans);
        return -1;
    }

    auto iter = g_http_trans_bucket_map.find(id);
    if (iter == g_http_trans_bucket_map.end()) {
        return -2;
    }

    std::transform(url.begin(), url.end(), url.begin(), ::tolower);
    auto iter2 = iter->second.find(url);
    if (iter2 == iter->second.end()) {
        return -3;
    }

    BaseTransaction* t = iter2->second->Create();
    if (!t) {
        assert(false);
        return -1;
    }

    auto ht = dynamic_cast<BaseHttpTransaction*>(t);
    if (!ht) {
        assert(false);
        return -1;
    }

    g_cur_concurrent_trans++;
    // 用来存储id
    ht->SetReqCmdId(id);
    ht->SetUrl(url);
    ht->Handle(packet, packet_size, ext);
    return 0;
}

void tick(uint32_t cur_time) {
    for (auto iter = g_tick_trans_list.begin(); iter != g_tick_trans_list.end(); ++iter) {
        (*iter)->Tick(cur_time);
    }
    if (cur_time - g_last_check_time >= 120) {
        g_last_check_time = cur_time;
        log("[transaction] [statistics] cur_trans:%d", g_cur_concurrent_trans);
    }
}

int regBucket(TransactionBucket* bucket) {
    auto iter = g_trans_bucket_map.find(bucket->ReqCmdId());
    if (iter != g_trans_bucket_map.end()) {
        delete bucket;
        assert(false);
        return -1;
    }

    log("[transaction] regist req_cmd:%d, rsp_cmd:%d, trans:%s", bucket->ReqCmdId(), bucket->RspCmdId(), bucket->TransName());
    g_trans_bucket_map.insert(std::make_pair(bucket->ReqCmdId(), bucket));
    return 0;
}

void recycleTrans(BaseTransaction* t) {
    do {
        // 优先试图转成BaseHttpTransaction
        auto ht = dynamic_cast<BaseHttpTransaction*>(t);

        if (ht) {
            auto iter = g_http_trans_bucket_map.find(t->ReqCmdId());
            if (iter == g_http_trans_bucket_map.end()) {
                assert(false);
                return;
            }
            auto iter2 = iter->second.find(ht->Url());
            if (iter2 == iter->second.end()) {
                return;
            }

            iter2->second->Recycle(t);
        } else {
            if (t->ReqCmdId() == 0 && t->RspCmdId() == 0) {
                return;
            }

            auto iter = g_trans_bucket_map.find(t->ReqCmdId());
            if (iter == g_trans_bucket_map.end()) {
                assert(false);
                return;          
            }

            iter->second->Recycle(t);
        }

    } while (false);

    g_cur_concurrent_trans--;
}

int regTickTrans(TransactionBucket* bucket) {
    BaseTransaction* t = bucket->Create();
    BaseTickTransaction* tt = dynamic_cast<BaseTickTransaction*>(t);
    g_tick_trans_list.push_back(tt);
    log("[transaction] regist tick %s", bucket->TransName());
    delete bucket;
    return 0;
}

int regHttpTrans(uint32_t id, std::string url, TransactionBucket* bucket) {
    auto iter = g_http_trans_bucket_map.find(id);
    if (iter == g_http_trans_bucket_map.end()) {
        std::unordered_map<std::string, TransactionBucket*> m;
        auto p = g_http_trans_bucket_map.insert(std::make_pair(id, m));
        iter = p.first;
    }

    std::transform(url.begin(), url.end(), url.begin(), ::tolower);
    auto iter2 = iter->second.find(url);
    if (iter2 != iter->second.end()) {
        delete bucket;
        assert(false);
        return -1;
    }

    log("[transaction] regist id:%d, url:%d, trans:%s", id, url.c_str(), bucket->TransName());
    iter->second.insert(std::make_pair(url, bucket));
    return 0;
}

}

#endif