/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "common/transaction/transaction_mgr.h"
#include <unordered_map>
#include <assert.h>

namespace trans_mgr {

/////////////////////////////////////////////////////////////////////////////////

uint32_t g_trans_id = 1;
uint32_t g_max_concurrent_trans = 100000;
uint32_t g_cur_concurrent_trans = 0;
std::unordered_map<uint32_t, TransactionBucket*> g_trans_bucket_map;
std::list<BaseTickTransaction*> g_tick_trans_list;

////////////////////////////////////////////////////////////////////////////////

uint32_t genTransId() {
    return (g_trans_id++);
}

void setMaxTrans(uint32_t max_trans) {
    g_max_concurrent_trans = max_trans;
}

int handle(uint32_t req_cmd_id, const char* packet, uint32_t packet_size) {
    if (g_cur_concurrent_trans >= g_max_concurrent_trans) {
        printf("over max concurrent trans limit:%d\n", g_cur_concurrent_trans);
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

    t->Handle(packet, packet_size);
    return 0;
}

void tick(uint32_t cur_time) {
    for (auto iter = g_tick_trans_list.begin(); iter != g_tick_trans_list.end(); ++iter) {
        (*iter)->Tick(cur_time);
    }
}

int registBucket(TransactionBucket* bucket) {
    auto iter = g_trans_bucket_map.find(bucket->ReqCmdId());
    if (iter != g_trans_bucket_map.end()) {
        delete bucket;
        assert(false);
        return -1;
    }

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