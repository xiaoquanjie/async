/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/async_redis.h"

#if CUR_REDIS_VERSION == 2

#include "common/async/redis/redis_exception.h"
#include "common/log.h"
#include <vector>
#include <queue>
#include <list>
#include <unordered_map>
#include <hiredis-vip/adapters/libevent.h>
#include <hiredis-vip/hiredis.h>
#include <hiredis-vip/hircluster.h>
#include <assert.h>

namespace async {
// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace redis {

// redis连接状态
enum {
    enum_connecting_state = 0,
    enum_connected_state,
    enum_authing_state,
    enum_auth_state,        // 已认证通过
    enum_selecting_state,
    enum_selected_state,
    enum_disconnected_state,
};

// 地址结构
struct RedisAddr {
    std::string id;
    std::string host; // 集群：host1:port1,host2:port2 非集群:host
    uint32_t port = 0;
    std::string pwd; // 密码
    uint32_t dbIdx = 0;
    bool cluster = false;

    RedisAddr() {}

    RedisAddr(const std::string& id) {
        parse(id);
    }

    void parse(const std::string& id) {
        this->id = id;
        
        std::vector<std::string> values;
        async::split(id, "|", values);

        if (values.size() == 5) {
            this->pwd = values[2];
            this->dbIdx = atoi(values[3].c_str());
            this->cluster = (bool)atoi(values[4].c_str());
        
            if (this->cluster) {
                std::vector<std::string> hostVec;
                std::vector<std::string> portVec;
                async::split(values[0], ",", hostVec);
                async::split(values[1], ",", portVec);
                if (hostVec.size() == portVec.size()) {
                    std::string host;
                    for (size_t idx = 0; idx < hostVec.size(); idx++) {
                        if (!host.empty()) {
                            host += ",";
                        }
                        host += hostVec[idx] + ":" + portVec[idx];
                    }    
                    this->host = host;
                }
            } else {
                this->host = values[0];
                this->port = atoi(values[1].c_str());
            }
        } else {
            assert(false);
        }
    }
};

typedef std::shared_ptr<RedisAddr> RedisAddrPtr;

// 请求
struct RedisReqData {
    async_redis_cb cb;
    BaseRedisCmd cmd;
    RedisAddrPtr addr;
    time_t reqTime = 0;
};

typedef std::shared_ptr<RedisReqData> RedisReqDataPtr;

// 回复
struct RedisRspData {
    redisReply* reply = 0;
    RedisReqDataPtr req;
};

typedef std::shared_ptr<RedisRspData> RedisRspDataPtr;

// redis连接核心
struct RedisCore {
    void *ctxt = 0;
    RedisAddrPtr addr;
    uint32_t state = enum_connecting_state;
    RedisReqDataPtr curReq;

    ~RedisCore() {
        if (this->ctxt) {
            // 断开连接
            if (addr->cluster) {
                redisClusterAsyncSetDisconnectCallback((redisClusterAsyncContext *)this->ctxt, 0);
                redisClusterAsyncDisconnect((redisClusterAsyncContext *)this->ctxt);
            } else {
                redisAsyncSetDisconnectCallback((redisAsyncContext *)this->ctxt, 0);
                redisAsyncDisconnect((redisAsyncContext *)this->ctxt);
            }
        }
    }

private:
    // RedisCore& operator=(const RedisCore&) = delete;
    // RedisCore(const RedisCore&) = delete;
};

typedef std::shared_ptr<RedisCore> RedisCorePtr;

struct CorePool {
    time_t lastCreate = 0;            // 上一次创建core的时间
    time_t lastDisconnect = 0;        // 上一次断联时间
    std::list<RedisCorePtr> valid;      // 可用的
    std::list<RedisCorePtr> conning;    // 正在连接的队列
    std::unordered_map<void*, RedisCorePtr>  useing;   // 正在使用中的, using是关键字
};

struct RedisThreadData {
    // 事件核心
    event_base* evtBase = 0;
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<RedisReqDataPtr> reqQueue;
    std::list<RedisReqDataPtr> prepareQueue;
    // 回复队列
    std::queue<RedisRspDataPtr> rspQueue;
    // 连接池
    std::unordered_map<std::string, CorePool> corePool;
    // 标记
    bool running = false;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
};

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 最大连接数，连接数越大表现越好
uint32_t gMaxConnection = 100;
// 超时时间
uint32_t gTimeout = 15;

RedisThreadData* runningData() {
    thread_local static RedisThreadData gThrData;
    return &gThrData;
}

#define redislLog(format, ...) log("[async_redis] " format, __VA_ARGS__)

void localInit(RedisThreadData* tData) {
    if (!tData->evtBase) {
        tData->evtBase = event_base_new();
    }
    if (!tData->evtBase) {
        redislLog("[error] failed to call event_base_new%s", "");
    }
}

// 拷贝一份redisreply
redisReply* copyRedisReply(redisReply* reply) {
	redisReply* r = (redisReply*)calloc(1, sizeof(*r));
	memcpy(r, reply, sizeof(*r));

    //copy str
	if(REDIS_REPLY_ERROR==reply->type 
        || REDIS_REPLY_STRING==reply->type 
        || REDIS_REPLY_STATUS==reply->type)  {
		r->str = (char*)malloc(reply->len+1);
		memcpy(r->str, reply->str, reply->len);
		r->str[reply->len] = '\0';
	}
    //copy array
	else if(REDIS_REPLY_ARRAY==reply->type) {
		r->element = (redisReply**)calloc(reply->elements, sizeof(redisReply*));
		memset(r->element, 0, r->elements*sizeof(redisReply*));
		for(uint32_t i=0; i<reply->elements; ++i) {
			if(NULL!=reply->element[i]) {
				if( NULL == (r->element[i] = copyRedisReply(reply->element[i])) ) {
					//clone child failed, free current reply, and return NULL
					freeReplyObject(r);
					return NULL;
				}
			}
		}
	}
	return r;
}

/////////////////////////////// 响应连接 ////////////////////////////
void onThreadRedisConnect(RedisCore* core, int status) {
    if (!core) {
        redislLog("[error] not recognized rediscore: %p connection", core);
        return;
    }

    if (status == REDIS_OK) {
        core->state = enum_connected_state;
        redislLog("%p|%s connection is successful", core, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        redislLog("[error] %p|%s connection is failed", core, core->addr->id.c_str());
        core->ctxt = 0;
    }
}
void onThreadClusterConnect(const redisAsyncContext* c, int status) {
    const redisClusterAsyncContext* cc = (const redisClusterAsyncContext*)c;
    RedisCore* core = (RedisCore*)cc->data;
    onThreadRedisConnect(core, status);
}
void onThreadSingleConnect(const redisAsyncContext *c, int status) {
    RedisCore* core = (RedisCore*)c->data;
    onThreadRedisConnect(core, status);
}
/////////////////////////////// 响应连接 end ////////////////////////////

/////////////////////////////// 响应断线 ///////////////////////////////
void onThreadRedisDisconnect(RedisCore* core, int) {
    if (!core) {
        redislLog("[error] not recognized rediscore: %p disconnection", core);
        return;
    }

    core->state = enum_disconnected_state;
    redislLog("%p|%s is connection is breaking", core, core->addr->id.c_str());
    core->ctxt = 0;
}
void onThreadClusterDisconnect(const redisAsyncContext* c, int status) {
    const redisClusterAsyncContext* cc = (const redisClusterAsyncContext*)c;
    RedisCore* core = (RedisCore*)cc->data;
    onThreadRedisDisconnect(core, status);
}
void onThreadSingleDisconnect(const redisAsyncContext *c, int status) {
    RedisCore* core = (RedisCore*)c->data;
    onThreadRedisDisconnect(core, status);
}
/////////////////////////////// 响应断线 end ///////////////////////////////

// //////////////////////////// 响应授权///////////////////////////////////
void onThreadRedisAuth(RedisCore* core, void* r, void* uData) {
    if (!core) {
        redislLog("[error] not recognized rediscore: %p connection", core);
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        core->state = enum_auth_state;
        redislLog("rediscore:%p|%s auth successfully", core, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        redislLog("[error] failed to auth:%p|%s|d", core, core->addr->pwd.c_str(), reply->type);
    }
}
void onThreadClusterAuth(redisClusterAsyncContext* c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisAuth(core, r, uData);
}
void onThreadSingleAuth(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisAuth(core, r, uData);
}
////////////////////////////// 响应授权 end///////////////////////////////////

////////////////////////////// 响应选库 //////////////////////////////////////
void onThreadRedisSelect(RedisCore* core, void* r, void* uData) {
    if (!core) {
        redislLog("[error] not recognized rediscore: %p connection", core);
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        core->state = enum_selected_state;
        redislLog("rediscore:%p|%s select db successfully", core, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        redislLog("[error] failed to select db:%p|%s|%d", core, core->addr->pwd.c_str(), reply->type);
    }
}
void onThreadClusterSelect(redisClusterAsyncContext* c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisSelect(core, r, uData);
}

void onThreadSingleSelect(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisSelect(core, r, uData);
}
////////////////////////////// 响应选库 end //////////////////////////////////////

////////////////////////////// 响应结果 //////////////////////////////////////////
void onThreadRedisResult(void* c, RedisCore* core, void* r, void* uData) {
    RedisThreadData* tData = (RedisThreadData*)uData;
    if (!tData) {
        redislLog("[error] no redis_thread_data%s", "");
        return;
    }

    if (!core) {
        redislLog("[error] not recognized rediscore: %p connection", core);
        return;
    }

    if (!core->curReq) {
        redislLog("[error] no current redis_req_data%s", "");
        return;
    }

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->reply = copyRedisReply((redisReply*)r);
    rsp->req = core->curReq;
    tData->rspQueue.push(rsp);

    core->curReq = nullptr;
    
    // 归还core到队列
    auto id = core->addr->id;
    auto& pool = tData->corePool[id];
    // 从使用队列删除
    auto coreIter = pool.useing.find(c);
    if (coreIter != pool.useing.end()) {
        pool.valid.push_back(coreIter->second);
        pool.useing.erase(coreIter);
    }
}
void onThreadClusterResult(redisClusterAsyncContext* c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisResult(c, core, r, uData);
}
void onThreadSingleResult(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    onThreadRedisResult(c, core, r, uData);
}
////////////////////////////// 响应结果 //////////////////////////////////////////

// 响应超时
void onThreadRedisTimeout(RedisThreadData* tData, RedisReqDataPtr req) {
    redisReply* r = (redisReply*)calloc(1, sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->type = REDIS_REPLY_SELF_TIMEOUT;

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->reply = r;
    rsp->req = req;
    tData->rspQueue.push(rsp);
}

RedisCorePtr threadCreateRedisCore(RedisThreadData* tData, RedisAddrPtr addr) {
    auto& corePool = tData->corePool[addr->id];
    if (!corePool.valid.empty()) {
        auto core = corePool.valid.front();
        core->curReq = nullptr;
        corePool.valid.pop_front();
        corePool.useing[core->ctxt] = core;
        return core;
    }

    if (corePool.conning.size()) {
        return nullptr;
    }

    uint32_t conns = corePool.useing.size() + corePool.conning.size();
    if (conns >= gMaxConnection) {
        return nullptr;
    }

    time_t curTime = 0;
    time(&curTime);

    // 不允许短时间内快速创建多个相同断联连接
    if (corePool.lastDisconnect == curTime) {
        return nullptr;
    }

    corePool.lastCreate = curTime;

    RedisCorePtr core = std::make_shared<RedisCore>();
    core->addr = addr;
   
    if (core->addr->cluster) {
        redisClusterAsyncContext* clusterC = redisClusterAsyncConnect(core->addr->host.c_str(), HIRCLUSTER_FLAG_NULL);
        if (clusterC->err) {
            redislLog("[error] failed to call redisClusterAsyncConnect: %s", clusterC->errstr);
            return nullptr;
        }
        if (clusterC) {
            clusterC->data = core.get();
            core->ctxt = clusterC;
        } else {
            redislLog("[error] failed to call redisClusterAsyncConnect: %s", core->addr->host.c_str());
        }
    } else {
        redisAsyncContext* c = redisAsyncConnect(core->addr->host.c_str(), core->addr->port);
        if (c) {
            c->data = core.get();
            core->ctxt = c;
        } else {
            redislLog("[error] failed to call redisAsyncConnect: %s:%d", core->addr->host.c_str(), core->addr->port);
        }
    }
    
    if (!core->ctxt) {
        return nullptr;
    }
    
    corePool.conning.push_back(core);

    // set callback
    if (core->addr->cluster) {
        redisClusterLibeventAttach((redisClusterAsyncContext *)core->ctxt, tData->evtBase);
        redisClusterAsyncSetConnectCallback((redisClusterAsyncContext *)core->ctxt, onThreadClusterConnect);
        redisClusterAsyncSetDisconnectCallback((redisClusterAsyncContext *)core->ctxt, onThreadClusterDisconnect);
    } else {
        redisLibeventAttach((redisAsyncContext *)core->ctxt, tData->evtBase);
        redisAsyncSetConnectCallback((redisAsyncContext *)core->ctxt, onThreadSingleConnect);
        redisAsyncSetDisconnectCallback((redisAsyncContext *)core->ctxt, onThreadClusterDisconnect);       
    }

    return nullptr;
}

void threadRedisOp(void* c, bool cluster, void(*fn)(void*, void*, void*), void* uData, const BaseRedisCmd& cmd) {
    size_t cmdSize = cmd.cmd.size();
    std::vector<const char *> argv(cmdSize);
    std::vector<size_t> argc(cmdSize);

    for (size_t idx = 0; idx < cmdSize; ++idx) {
        if (cmd.cmd[idx].empty()) {
            argv[idx] = 0;
        }
        else {
            argv[idx] = cmd.cmd[idx].c_str();
        }
        argc[idx] = cmd.cmd[idx].length();
    }

    if (cluster) {
        redisClusterAsyncCommandArgv((redisClusterAsyncContext*)c,
                                     (void (*)(redisClusterAsyncContext*, void*, void*))fn,
                                     (void *)uData,
                                     cmdSize,
                                     &(argv[0]),
                                     &(argc[0]));
    } else {
         redisAsyncCommandArgv((redisAsyncContext*)c,
                              (void (*)(redisAsyncContext*, void*, void*))fn,
                              (void *)uData,
                              cmdSize,
                              &(argv[0]),
                              &(argc[0]));
    }
}

// 状态查询
void threadRedisState(RedisThreadData* tData) {
    for (auto& item : tData->corePool) {
        auto& pool = item.second;
        for (auto iter = pool.valid.begin(); iter != pool.valid.end(); ) {
            auto& core = (*iter);
            if (core->state == enum_disconnected_state) {
                time(&pool.lastDisconnect);
                iter = pool.valid.erase(iter);
            } else {
                iter++;
            }
        }

        for (auto iter = pool.conning.begin(); iter != pool.conning.end(); ) {
            auto& core = (*iter);
            if (core->state == enum_disconnected_state) {
                time(&pool.lastDisconnect);
                iter = pool.conning.erase(iter);
                continue;
            }  
            
            int32_t needStat = 0;

            if (core->state == enum_connected_state) {
                // 授权
                if (core->addr->pwd.empty()) {
                    core->state = enum_selecting_state;
                    needStat = 1;
                } else {
                    core->state = enum_authing_state;
                    needStat = 2;
                }
            } else if (core->state == enum_auth_state) {
                core->state = enum_selecting_state;
                needStat = 1;
            } else if (core->state == enum_selected_state) {
                // 挪入可用队列
                pool.valid.push_back(core);
                iter = pool.conning.erase(iter);
                continue;
            }

            if (needStat == 1) {
                if (core->addr->cluster) {
                    threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadClusterSelect, tData, SelectCmd(core->addr->dbIdx));
                } else {
                    threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadSingleSelect, tData, SelectCmd(core->addr->dbIdx));
                }
            } else if (needStat == 2) {
                if (core->addr->cluster) {
                    threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadClusterAuth, tData, AuthCmd(core->addr->pwd));
                } else {
                    threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadSingleAuth, tData, AuthCmd(core->addr->pwd));
                }
            }

            iter++;
        }
    }
}

void threadPrepare(RedisThreadData* tData) {
    time_t curTime = 0;
    time(&curTime);

    for (auto iter = tData->prepareQueue.begin(); iter != tData->prepareQueue.end(); ) {
        RedisReqDataPtr req = (*iter);
        if (curTime - req->reqTime > gTimeout) {
            onThreadRedisTimeout(tData, req);
            iter = tData->prepareQueue.erase(iter);
            continue;
        }

        auto core = threadCreateRedisCore(tData, req->addr);
        if (core) {
            core->curReq = req;
            if (core->addr->cluster) {
                threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadClusterResult, tData, core->curReq->cmd);
            } else {
                threadRedisOp(core->ctxt, core->addr->cluster, (void(*)(void*, void*, void*))onThreadSingleResult, tData, core->curReq->cmd);
            }
            iter = tData->prepareQueue.erase(iter);
            continue;
        }

        break;
    }
}

void threadProcess(RedisThreadData* tData) {
    threadPrepare(tData);
    threadRedisState(tData);
    //redislLog(".......%s", "");
    event_base_loop(tData->evtBase, EVLOOP_NONBLOCK);
    tData->running = false;
}

void localRequest(RedisThreadData* tData) {
    if (!tData->reqQueue.empty()) {
        tData->prepareQueue.insert(tData->prepareQueue.end(), tData->reqQueue.begin(), tData->reqQueue.end());
        tData->reqQueue.clear();
    }
}

void localRespond(RedisThreadData* tData) {
    while (!tData->rspQueue.empty()) {
        tData->rspTaskCnt++;
        RedisRspDataPtr rsp = tData->rspQueue.front();
        tData->rspQueue.pop();
        RedisReplyParserPtr parser = std::make_shared<RedisReplyParser>(rsp->reply);
        rsp->req->cb(parser);
    }
}

void localProcess(uint32_t curTime, RedisThreadData* tData) {
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

void localStatistics(int32_t curTime, RedisThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    redislLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    
    for (auto& item : tData->corePool) {
        redislLog("[statistics] id: %s, valid: %d, conning: %d, using: %d", item.first.c_str(), item.second.valid.size(), item.second.conning.size(), item.second.useing.size());
    }
}

///////////////////////////////////////////// public //////////////////////////////

// @uri: [host|port|pwd|idx|cluster]
void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb) {
    RedisThreadData* tData = runningData();
    localInit(tData);

    RedisReqDataPtr req = std::make_shared<RedisReqData>();
    req->cmd = redis_cmd;
    req->cb = cb;
    req->addr = std::make_shared<RedisAddr>(uri);
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

// 同步redis接口, 无连接池
//bool executeSync(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb);

bool loop(uint32_t curTime) {
    RedisThreadData* tData = runningData();
    if (!tData->evtBase) {
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

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

} // redis
} // async

#endif
#endif