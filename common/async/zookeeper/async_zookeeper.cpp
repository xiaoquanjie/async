/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "common/async/zookeeper/data.h"
#include "common/async/zookeeper/callback.h"
#include "common/async/zookeeper/op.h"
#include <cassert>
#include <set>

namespace async {

uint32_t supposeIothread();
clock_t getMilClock();

namespace zookeeper {

#define IO_THREADS async::supposeIothread()

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 10;
// 挂载任务数
uint32_t gMountTask = 10;

//////////////////////////////////////////////////////////////////////////////////////////

ZookThreadData* runningData() {
    thread_local static ZookThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

// init log level
struct InitLogLevel {
    InitLogLevel() {
        zooSetDebugLevel(ZOO_LOG_LEVEL_ERROR);
    }
} g_init_log_level;

//////////////////////////////////////////////////////////////////////////////////////////

ZookCorePtr localCreateCore(ZookAddrPtr addr) {
    auto core = std::make_shared<ZookCore>();
    core->addr = addr;
    return core;
}

ZookConnPtr threadCreateConn(ZookCorePtr core) {
    auto conn = std::make_shared<ZookConn>();
    conn->zh = zookeeper_init(core->addr->host.c_str(), onConnCb, 10 * 1000, 0, core.get(), 0);
    if (!conn->zh) {
        zookLog("[error] failed to call zookeeper_init: %s", core->addr->id.c_str());
        return nullptr;
    } else {
        zookLog("create connection: %s", core->addr->id.c_str());
    }

    core->conn = std::make_shared<ZookConn>();
    return conn;
}

void threadPoll(std::list<zhandle_t*>& zhList) {
    if (zhList.empty()) {
        return;
    }

    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    struct FD_INFO {
        zhandle_t* zh;
        int fd;
    };

    int maxFd = 0;
    FD_INFO clientFd[FD_SETSIZE];
    int idx = 0;

    for (auto zh : zhList) {
        if (idx + 1 == FD_SETSIZE) {
            continue;
        }

        struct timeval tv;
        int fd = 0;
        int interest;

        zookeeper_interest(zh, &fd, &interest, &tv);
        if (fd > 0) {
            if (interest & ZOOKEEPER_READ) {
                FD_SET(fd, &rfds);
            } else {
                FD_CLR(fd, &rfds);
            }
            if (interest & ZOOKEEPER_WRITE) {
                FD_SET(fd, &wfds);
            } else {
                FD_CLR(fd, &wfds);
            }
        }

        if (fd != 0) {
            auto i = idx++;
            clientFd[i].fd = fd;
            clientFd[i].zh = zh;
            if (fd > maxFd) {
                maxFd = fd;
            }
        }
    }

    if (idx == 0) {
        return;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int rc = select(maxFd + 1, &rfds, &wfds, &efds, &tv);
    for (int i = 0; i < rc; i++) {
        int events = 0;
        if (FD_ISSET(clientFd[i].fd, &rfds)) {
            events |= ZOOKEEPER_READ;
        }
        if (FD_ISSET(clientFd[i].fd, &wfds)) {
            events |= ZOOKEEPER_WRITE;
        }
        zookeeper_process(clientFd[i].zh, events);
    }
}

void threadProcess(std::list<ZookCorePtr> coreList) {
    time_t curTime =0;
    time(&curTime);

    std::list<zhandle_t*> zhList;

    for (auto c : coreList) {
        if (!c->conn) {
            c->conn = threadCreateConn(c);
        }
        else if (c->conn->connected) {
            if (c->conn->auth == 0) {
                // 进行验证
                if (c->addr->scheme.empty()) {
                    c->conn->auth = 1;
                } else {
                    c->conn->auth = -1;
                    if (!checkOk(zoo_add_auth(c->conn->zh, 
                        "digest",
                        c->addr->scheme.c_str(), 
                        0, 
                        onAuthCb, 
                        c.get()), "zoo_add_auth")
                    ) {
                        //
                        c->conn->auth = 0;
                    }
                }
            }
        } 

        if (c->conn) {
            zhList.push_back(c->conn->zh);
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
            if (c->conn && c->conn->auth == 1) {
                // 操作
                //zookLog("op...%s", "");
                opCmd(c, req);
                iter = c->waitQueue.erase(iter);
            } else {
                break;
            }
        } 
        c->waitLock.unlock();
    }

    threadPoll(zhList);

    // 去掉运行标识
    for (auto c : coreList) {
        c->running = false;
    }    
}

//////////////////////////////////////////////////////////////////////////////////////////

void localProcess(uint32_t curTime, ZookThreadData* tData) {
     if (tData->reqQueue.empty()) {
        return;
    }

    uint32_t ioThreads = IO_THREADS;
    auto gData = globalData();
    gData->pLock.lock();

    do {
        for (auto iterQueue = tData->reqQueue.begin(); iterQueue != tData->reqQueue.end(); iterQueue++) {
            auto& id = iterQueue->first;
            auto& que = iterQueue->second;
            auto pool = gData->getPool(id);

            for (auto iterReq = que.begin(); iterReq != que.end(); ) {
                auto req = *iterReq;
                if (curTime - req->reqTime >= gTimeout) {
                    iterReq = que.erase(iterReq);
                    onTimeout(req, nullptr, tData);
                    continue;
                }

                // 遍历一个有效的core
                ZookCorePtr core;
                if (ioThreads == pool->valid.size()) {
                    // 均匀分布
                    core = pool->valid[pool->polling++ % ioThreads];
                }
                else {
                    for (auto c : pool->valid) {
                        if (c->task < gMountTask) {
                            core = c;
                            break;
                        }
                    }
                }
                
                if (core) {
                    core->waitLock.lock();
                    core->waitQueue.push_back(req);
                    core->waitLock.unlock();
                    core->task++;
                    iterReq = que.erase(iterReq);
                    continue;
                }

                // 建新core
                if (pool->valid.size() < ioThreads) {
                    auto c = localCreateCore(req->addr);
                    if (c) {
                        pool->valid.push_back(c);
                    }
                }
            }
        }
    } while (false);

    gData->pLock.unlock();
}

void localRespond(ZookThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<ZookRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        tData->rspTaskCnt++;
        rsp->req->cb(rsp->parser);
    }
}

void localStatistics(int32_t curTime, ZookThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    zookLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->pLock.lock();
    for (auto& pool : gData->corePool) {
        zookLog("[statistics] id: %s, valid: %d", pool.first.c_str(), pool.second->valid.size());
    }
    gData->pLock.unlock();
}

void localDispatchTask(ZookThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    clock_t nowClock = getMilClock();

    // 把所有的core分为IO_THREADS组
    uint32_t count = IO_THREADS;
    std::vector<std::list<ZookCorePtr>> coreGroup(count);
    uint32_t groupIdx = 0;

    // 加锁
    gData->pLock.lock();

    for (auto& poolItem : gData->corePool) {
        auto pool = poolItem.second;
        for (auto c : pool->valid) {
            if (c->running) {
                continue;
            }
            if (c->task == 0 && nowClock -  c->lastRunTime <= 500) {
                continue;
            }
            //zookLog("running......%s", c->addr->id.c_str());
            c->running = true;
            c->lastRunTime = nowClock;
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
        //      zookLog("distpatch task to group: %d and core_nums: %d, task: %d, core: %p", idx, g.size(), c->task.load(), c.get());
        // }
        
        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, g);
            gIoThr(f);
        } else {
            threadProcess(g);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

void execute(const std::string& uri, std::shared_ptr<BaseZookCmd> cmd, async_zookeeper_cb cb) {
    auto tData = runningData();
    tData->init = true;

    auto req = std::make_shared<ZookReqData>();
    req->cb = cb;
    req->cmd = cmd;
    req->addr = std::make_shared<ZookAddr>(uri);
    req->tData = tData;
    time(&req->reqTime);

    auto& que = tData->reqQueue[uri];
    que.push_back(req);
    tData->reqTaskCnt++;
    //zookLog("post...%s", "");
}

bool loop(uint32_t curTime) {
    auto tData = runningData();
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

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

void zooSetDebugLevel(uint32_t level) {
    // typedef enum {ZOO_LOG_LEVEL_ERROR=1,ZOO_LOG_LEVEL_WARN=2,ZOO_LOG_LEVEL_INFO=3,ZOO_LOG_LEVEL_DEBUG=4} ZooLogLevel;
    zoo_set_debug_level((ZooLogLevel)level);
}

}
}

#endif