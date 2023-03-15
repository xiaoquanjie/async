/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/data.h"

namespace async {
namespace mysql {

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 最大连接数，连接数越大表现越好
uint32_t gMaxConnection = 100;
// 超时时间
uint32_t gTimeout = 5;

///////////////////////////////////////////////////////////////////////////////

MysqlThreadData* runningData() {
    thread_local static MysqlThreadData gThrData;
    return &gThrData;
}

MysqlCorePtr threadCreateCore(MysqlThreadData* tData, MysqlAddrPtr addr) {
    // 取空闲的
    auto& pool = tData->corePool[addr->id];
    if (!pool.valid.empty()) {
        auto core = pool.valid.front();
        core->curReq = nullptr;
        pool.valid.pop_front();
        core->state = enum_querying_state;
        pool.useing.push_back(core);
        return core;
    }

    if (pool.conning.size()) {
        return nullptr;
    }

    // 超出最大连接数
    uint32_t conns = pool.useing.size() + pool.conning.size();
    if (conns >= gMaxConnection) {
        return nullptr;
    }

    time_t curTime = 0;
    time(&curTime);

    // 不允许短时间内快速创建多个相同断联连接
    if (pool.lastCreate == curTime) {
        return nullptr;
    }

    pool.lastCreate = curTime;

    // 创建新连接
    MysqlCorePtr core = std::make_shared<MysqlCore>();
    core->addr = addr;
    core->c = mysql_init(NULL);
    if (!core->c) {
        mysqlLog("[error] failed to call mysql_init%s", "");
        return nullptr;
    }

    net_async_status status = mysql_real_connect_nonblocking(core->c, 
        addr->host.c_str(), 
        addr->user.c_str(),
        addr->pwd.c_str(), 
        addr->db.c_str(), 
        addr->port,
        0, 
        0);

    if (NET_ASYNC_ERROR == status) {
        mysqlLog("[error] failed to call mysql_real_connect_nonblocking:[%s]|%s", addr->id.c_str(), mysql_error(core->c));
        return nullptr;
    }

    pool.conning.push_back(core);
    return nullptr;
}

void threadOnResult(MysqlThreadData* tData, MysqlCorePtr core, MysqlReqDataPtr req, const void* res, bool timeout) {
    MysqlRspDataPtr rsp = std::make_shared<MysqlRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<MysqlReplyParser>(res, 0);
    if (timeout) {
        rsp->parser->_timeout = timeout;
    } else {
        rsp->parser->_errno = mysql_errno(core->c);
        if (rsp->parser->_errno != 0) {
            mysqlLog("[error] mysql err msg: %s", mysql_error(core->c));
        } else {
            rsp->parser->_affect = mysql_affected_rows(core->c); 
        }
    }
    tData->rspQueue.push_back(rsp);
}

void threadPrepare(MysqlThreadData* tData) {
    time_t curTime = 0;
    time(&curTime);

    for (auto iter = tData->prepareQueue.begin(); iter != tData->prepareQueue.end(); ) {
        MysqlReqDataPtr req = (*iter);
        if (curTime - req->reqTime > gTimeout) {
            threadOnResult(tData, nullptr, req, nullptr, true);
            iter = tData->prepareQueue.erase(iter);
            continue;
        }

        MysqlCorePtr core = threadCreateCore(tData, req->addr);
        if (core) {
            core->curReq = req;
            iter = tData->prepareQueue.erase(iter);
        }
        else {
            iter++;
        }
    }
}

void threadMysqlState(MysqlThreadData* tData) {
    for (auto& item : tData->corePool) {
        auto& pool = item.second;

        // connecting中的
        for (auto iter = pool.conning.begin(); iter != pool.conning.end(); ) {
            auto& core = (*iter);
            net_async_status status = mysql_real_connect_nonblocking(core->c, 
                core->addr->host.c_str(), 
                core->addr->user.c_str(),
                core->addr->pwd.c_str(), 
                core->addr->db.c_str(), 
                core->addr->port,
                0, 
                0
            );
            // 未准备好
            if (status == NET_ASYNC_NOT_READY) {
                iter++;
                continue;
            }
            // 成功
            if (status != NET_ASYNC_ERROR) {
                // 挪入可用队列
                core->state = enum_connected_state;
                pool.valid.push_back(core);
                mysqlLog("mysql:[%s] connect successfully", core->addr->id.c_str());
            }
            else {
                // 失败
                mysqlLog("[error] failed to call mysql_real_connect_nonblocking: [%s]|%s", core->addr->id.c_str(), mysql_error(core->c));
            }
            iter = pool.conning.erase(iter);
        }

        // using中的
        for (auto iter = pool.useing.begin(); iter != pool.useing.end(); ) {
            auto& core = (*iter);
            // 刚连接上
            if (core->state == enum_querying_state) {
                net_async_status status = mysql_real_query_nonblocking(core->c, 
                    core->curReq->cmd.c_str(),
                    (unsigned long)core->curReq->cmd.size()
                );
                // 准备好
                if (status != NET_ASYNC_NOT_READY) {
                    // 成功
                    if (status != NET_ASYNC_ERROR) {
                        core->state = enum_storing_state;
                        //mysqlLog("query ok: %s", core->curReq->cmd.c_str());
                    } else {
                        // 失败
                        core->state = enum_error_state;
                        threadOnResult(tData, core, core->curReq, nullptr, false);
                    }
                }
            }

            // 刚查询完
            if (core->state == enum_storing_state) {
                MYSQL_RES *result = 0;
                net_async_status status = mysql_store_result_nonblocking(core->c, &result);
                // 准备好
                if (status != NET_ASYNC_NOT_READY) {
                    // 成功
                    if (status != NET_ASYNC_ERROR) {
                        // 拿到结果了, 结果已全部存到MYSQL_RES
                        threadOnResult(tData, core, core->curReq, result, false);
                        // 归还core
                        core->state = enum_connected_state;
                        core->curReq = nullptr;
                        pool.valid.push_back(core);
                        iter = pool.useing.erase(iter);
                        continue;
                    } else {
                        // 失败
                        core->state = enum_error_state;
                        threadOnResult(tData, core, core->curReq, nullptr, false);
                    }
                }
            }

            // 发生了错误
            if (core->state == enum_error_state) {
                mysqlLog("[error] failed to call mysql_real_query_nonblocking: [%s]|%s", core->addr->id.c_str(), mysql_error(core->c));
                iter = pool.useing.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

void threadProcess(MysqlThreadData* tData) {
    threadPrepare(tData);
    threadMysqlState(tData);
    tData->running = false;
}

// 构造一个请求
void localCreateReq(const std::string& id, const std::string& sql, async_mysql_cb cb, bool isQuery) {
    MysqlThreadData* tData = runningData();
    tData->isInit = true;

    MysqlReqDataPtr req = std::make_shared<MysqlReqData>();
    req->cmd = sql;
    req->cb = cb;
    req->isQuery = isQuery;
    req->addr = std::make_shared<MysqlAddr>(id);
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

void localStatistics(int32_t curTime, MysqlThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    mysqlLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    for (auto& item : tData->corePool) {
        mysqlLog("[statistics] id: %s, valid: %d, conning: %d, using: %d", item.first.c_str(), item.second.valid.size(), item.second.conning.size(), item.second.useing.size());
    }
}

void localRespond(MysqlThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }
    for (auto iter = tData->rspQueue.begin(); iter != tData->rspQueue.end(); ++iter) {
        tData->rspTaskCnt++;
        auto& rsp = *iter;
        rsp->req->cb(rsp->parser);
    }
    tData->rspQueue.clear();
}

void localRequest(MysqlThreadData* tData) {
    if (!tData->reqQueue.empty()) {
        tData->prepareQueue.insert(tData->prepareQueue.end(), tData->reqQueue.begin(), tData->reqQueue.end());
        tData->reqQueue.clear();
    }
}

void localProcess(uint32_t curTime, MysqlThreadData* tData) {
    if (tData->reqTaskCnt != tData->rspTaskCnt || curTime - tData->lastRunTime > 1) {
        tData->running = true;
        tData->lastRunTime = curTime;
        std::function<void()> f = std::bind(threadProcess, tData);
        if (gIoThr) {
            gIoThr(f);
        } else {
            f();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void query(const std::string& id, const std::string& sql, async_mysql_cb cb) {
    localCreateReq(id, sql, cb, true);
}

void execute(const std::string& id, const std::string& sql, async_mysql_cb cb) {
    localCreateReq(id, sql, cb, false);
}

void setMaxConnection(uint32_t cnt) {
    gMaxConnection = cnt;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

bool loop(uint32_t curTime) {
    MysqlThreadData* tData = runningData();
    if (!tData->isInit) {
        return false;
    }

    localStatistics(curTime, tData);

    if (!tData->running) {
        localRespond(tData);
        localRequest(tData);
        localProcess(curTime, tData);
    }

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}


}    
}

#endif