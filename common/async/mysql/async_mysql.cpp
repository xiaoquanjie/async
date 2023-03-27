/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/data.h"
#include "common/async/mysql/callback.h"

namespace async {

uint32_t supposeIothread();
clock_t getMilClock();

namespace mysql {

#define IO_THREADS async::supposeIothread()

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 10;
// 最大连接数，连接数越大表现越好
uint32_t gMaxConnection = 10;

///////////////////////////////////////////////////////////////////////////////

MysqlThreadData* runningData() {
    thread_local static MysqlThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

//////////////////////////////////////////////////////////////////////////////////////////

MysqlCorePtr localCreateCore(MysqlAddrPtr addr) {
    MysqlCorePtr core = std::make_shared<MysqlCore>();
    core->addr = addr;
    return core;
}

MysqlConnPtr threadCreateConn(MysqlCorePtr core) {
    auto conn = std::make_shared<MysqlConn>();
    conn->c = mysql_init(NULL);
    if (!conn->c) {
        mysqlLog("[error] failed to call mysql_init%s", "");
        return nullptr;
    }

    net_async_status status = mysql_real_connect_nonblocking(conn->c, 
        core->addr->host.c_str(), 
        core->addr->user.c_str(),
        core->addr->pwd.c_str(), 
        core->addr->db.c_str(), 
        core->addr->port,
        0, 
        0
    );

    if (NET_ASYNC_ERROR == status) {
        mysqlLog("[error] failed to call mysql_real_connect_nonblocking:[%s]|%s", core->addr->id.c_str(), mysql_error(conn->c));
        return nullptr;
    }

    return conn;
}

void threadPoll(MysqlCorePtr c) {
    if (c->conn->state == enum_connecting_state) {
        net_async_status status = mysql_real_connect_nonblocking(
            c->conn->c, 
            c->addr->host.c_str(), 
            c->addr->user.c_str(),
            c->addr->pwd.c_str(), 
            c->addr->db.c_str(), 
            c->addr->port,
            0, 
            0
        );

        // 未准备好
        if (status == NET_ASYNC_NOT_READY) {
            return;
        }

        // 成功
        if (status != NET_ASYNC_ERROR) {
            c->conn->state = enum_connected_state;
        }
        else {
            // 失败
            mysqlLog("[error] failed to call mysql_real_connect_nonblocking: [%s]|%s", 
                c->addr->id.c_str(), 
                mysql_error(c->conn->c));
            
            goto err;
        }
        return;
    }

    if (c->conn->state == enum_querying_state) {
        net_async_status status = mysql_real_query_nonblocking(
            c->conn->c, 
            c->curReq->cmd.c_str(),
            (unsigned long)c->curReq->cmd.size()
        );

        // 未准备好
        if (status == NET_ASYNC_NOT_READY) {
            return;
        }

        // 成功
        if (status != NET_ASYNC_ERROR) {
            c->conn->state = enum_storing_state;
        }
        else {
            // 失败
            mysqlLog("[error] failed to call mysql_real_connect_nonblocking: [%s]|%s", 
                c->addr->id.c_str(), 
                mysql_error(c->conn->c));
            
            onResult(c->curReq, c, nullptr);
            goto err;
        }
    }

    if (c->conn->state == enum_storing_state) {
        MYSQL_RES *result = 0;
        net_async_status status = mysql_store_result_nonblocking(c->conn->c, &result);

        // 未准备好
        if (status == NET_ASYNC_NOT_READY) {
            return;
        }

        // 成功
        if (status != NET_ASYNC_ERROR) {
            onResult(c->curReq, c, result);
            c->curReq = nullptr;
            c->conn->state = enum_connected_state;
        }
        else {
            // 失败
            mysqlLog("[error] failed to call mysql_store_result_nonblocking: [%s]|%s", 
                c->addr->id.c_str(), 
                mysql_error(c->conn->c));
            
            onResult(c->curReq, c, nullptr);
            goto err;
        }

        return;
    }

err:
    c->curReq = nullptr;
    c->conn = nullptr;
    return;
}

void threadProcess(std::list<MysqlCorePtr> coreList) {
    time_t curTime =0;
    time(&curTime);

    for (auto c : coreList) {
        if (!c->conn) {
            c->conn = threadCreateConn(c);
        }
        
        // 检查任务是否超时
        c->waitLock.lock();
        for (auto iter = c->waitQueue.begin(); iter != c->waitQueue.end();) {
            auto req = *iter;
            if (curTime - req->reqTime >= gTimeout) {
                onTimeout(req, c, req->tData);
                iter = c->waitQueue.erase(iter);
                continue;
            }
            if (c->conn && c->conn->state == enum_connected_state) {
                // 操作
                c->curReq = req;
                c->conn->state = enum_querying_state;
                iter = c->waitQueue.erase(iter);
            }
            else {
                break;
            }
        }
        c->waitLock.unlock();

        if (c->conn) {
            threadPoll(c);
        }
    }

    // 去掉运行标识
    for (auto c : coreList) {
        c->running = false;
    } 
}

//////////////////////////////////////////////////////////////////////////////////////////

void localStatistics(int32_t curTime, MysqlThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    mysqlLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->pLock.lock();
    for (auto& pool : gData->corePool) {
        mysqlLog("[statistics] id: %s, valid: %d", pool.first.c_str(), pool.second->valid.size());
    }
    gData->pLock.unlock();
}

void localRespond(MysqlThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<MysqlRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        tData->rspTaskCnt++;
        rsp->req->cb(rsp->parser);
    }
}

void localProcess(uint32_t curTime, MysqlThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    auto gData = globalData();
    gData->pLock.lock();
    for (auto iterQueue = tData->reqQueue.begin(); iterQueue != tData->reqQueue.end(); iterQueue++) {
        auto& id = iterQueue->first;
        auto& que = iterQueue->second;
        auto pool = gData->getPool(id);

        for (auto iterReq = que.begin(); iterReq != que.end(); iterReq++) {
            auto req = *iterReq;
            if (curTime - req->reqTime >= gTimeout) {
                onTimeout(req, nullptr, tData);
                continue;
            }

            MysqlCorePtr core;
            for (auto c : pool->valid) {
                if (c->task == 0) {
                    core = c;
                    break;
                }
            }

            if (!core) {
                if (pool->valid.size() < gMaxConnection) {
                    auto c = localCreateCore(req->addr);
                    pool->valid.push_back(c);
                }

                auto count = pool->valid.size();
                core = pool->valid[pool->polling++ % count];
            }

            core->waitLock.lock();
            core->waitQueue.push_back(req);
            core->waitLock.unlock();
            core->task++;
        }
        que.clear();
    }
    gData->pLock.unlock();
}

void localDispatchTask(MysqlThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    // 把所有的core分为IO_THREADS组
    uint32_t count = IO_THREADS;
    std::vector<std::list<MysqlCorePtr>> coreGroup(count);
    uint32_t groupIdx = 0;

    // 加锁
    gData->pLock.lock();
    for (auto& poolItem : gData->corePool) {
        auto pool = poolItem.second;
        for (auto c : pool->valid) {
            if (c->running) {
                continue;
            }
            if (c->conn && c->task == 0) {
                continue;
            }
           
            //mysqlLog("running......%s", c->addr->id.c_str());
            c->running = true;
            c->lastRunTime = 0;
            coreGroup[(groupIdx++) % count].push_back(c);
        }
    }
    // 解锁
    gData->pLock.unlock();

    // 清标记
    gData->dispatchTask.clear();   

    for (size_t idx = 0; idx < coreGroup.size(); idx++) {
        auto& g = coreGroup[idx];
        if (g.empty()) {
            continue;
        }
        
        // for test
        // for (auto c : g) {
        //      mysqlLog("distpatch task to group: %d and core_nums: %d, task: %d, core: %p", idx, g.size(), c->task.load(), c.get());
        // }
        
        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, g);
            gIoThr(f);
        } else {
            threadProcess(g);
        }
    } 
}

// 构造一个请求
void localCreateReq(const std::string& id, const std::string& sql, async_mysql_cb cb, bool isQuery) {
    MysqlThreadData* tData = runningData();
    tData->init = true;

    MysqlReqDataPtr req = std::make_shared<MysqlReqData>();
    req->cmd = sql;
    req->cb = cb;
    req->isQuery = isQuery;
    req->addr = std::make_shared<MysqlAddr>(id);
    req->tData = tData;
    time(&req->reqTime);

    auto& que = tData->reqQueue[id];
    que.push_back(req);
    tData->reqTaskCnt++;
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
    if (!tData->init) {
        return false;
    }

    localStatistics(curTime, tData);
    localRespond(tData);
    localProcess(curTime, tData);
    localDispatchTask(tData);

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}


}    
}

#endif