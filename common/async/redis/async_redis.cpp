/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/async_redis.h"
#include "common/async/redis/redis_exception.h"
#include <vector>
#include <queue>
#include <list>
#include <string.h>
#include <hiredis-vip/adapters/libevent.h>
#include <hiredis-vip/hiredis.h>
#include <hiredis-vip/hircluster.h>
#include <mutex>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace redis {

///////////////////////////////////////////////////////////////////////////////////////////////

// redis连接状态
enum {
    enum_connecting_state = 0,
    enum_connected_state,
    enum_selecting_state,
    enum_selected_state,
    enum_disconnected_state,
};

// redis地址结构
struct redis_addr
{
    // @uri: [host:port:pwd:idx:cluster]
    redis_addr(const std::string &uri) {
        Parse(uri);
    }

    redis_addr() {
    }

    void Parse(const std::string &uri) {
        // 解析uri
        this->uri = uri;
        std::vector<std::string> values;
        async::split(uri, "|", values);
        if (values.size() >= 5) {
            this->host = values[0];
            this->port = atoi(values[1].c_str());
            this->pwd = values[2];
            this->idx = atoi(values[3].c_str());
            this->cluster = atoi(values[4].c_str());
        }
    }

    std::string uri;
    std::string host;
    unsigned short port = 0;
    std::string pwd;
    unsigned short idx = 0;
    bool cluster = 0;
};

// redis连接核心
struct redis_core {
    void* ctxt = 0;
    redis_addr addr;
    int state = enum_connecting_state;

    redis_core() {}

    ~redis_core() {
        if (ctxt) {
            if (state != enum_disconnected_state) {
                // 断开连接，释放内存
                if (addr.cluster) {
                    redisClusterAsyncDisconnect((redisClusterAsyncContext *)ctxt);
                }
                else {
                    redisAsyncDisconnect((redisAsyncContext *)ctxt);
                }
            }
            // if (addr.cluster) {
            //     redisClusterAsyncFree((redisClusterAsyncContext *)ctxt);
            // }
            // else {
            //     redisAsyncFree((redisAsyncContext *)ctxt);
            // }
        }
    }

private:
    // 禁止拷贝
    redis_core& operator=(const redis_core&) = delete;
    redis_core(const redis_core&) = delete;
};

typedef std::shared_ptr<redis_core> redis_core_ptr;

struct redis_custom_data {
    redis_core_ptr core = nullptr;
    async_redis_cb cb;
    BaseRedisCmd cmd;
    redis_addr addr;
    time_t req_time = 0;
};

struct redis_respond_data {
    redisReply* reply = 0;
    redis_custom_data* data = 0;
};

struct redis_global_data {
    bool init = false;
    event_base* evt_base = 0;
    bool flag = false;
    std::function<void(std::function<void()>)> thr_func;
    std::queue<redis_custom_data*> request_queue;
    std::list<redis_custom_data*> prepare_queue;
    std::queue<redis_respond_data> respond_queue;
    std::map<std::string, redis_core_ptr> uri_map;
    std::map<std::string, time_t> uri_ct_map;
    uint64_t req_task_cnt = 0;
    uint64_t rsp_task_cnt = 0;
};

// 全局变量
redis_global_data g_redis_global_data;

time_t g_last_statistics_time = 0;

// 日志输出接口
std::function<void(const char*)> g_log_cb = [](const char* data) {
    static std::mutex s_mutex;
    s_mutex.lock();
    printf("[async_redis] %s\n", data);
    s_mutex.unlock();
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

//////////////////////////////////////////////////////////////////////////////////////

redisReply* copy_redis_reply(redisReply* reply) {
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
				if( NULL == (r->element[i] = copy_redis_reply(reply->element[i])) ) {
					//clone child failed, free current reply, and return NULL
					freeReplyObject(r);
					return NULL;
				}
			}
		}
	}
	return r;
}

void on_thread_redis_connect(const redisAsyncContext *c, int status) {
    bool flag = false;
    for (auto iter = g_redis_global_data.uri_map.begin(); iter != g_redis_global_data.uri_map.end(); ) {
        if (iter->second->ctxt == c) {
            log("%s connection|%p|uri:%s\n", (status == REDIS_OK ? "a new" : "a fail"),
                                   c,
                                   iter->second->addr.uri.c_str());
            if (status == REDIS_OK) {
                iter->second->state = enum_connected_state;
            }
            else {
                iter->second->state = enum_disconnected_state;
                // erase掉
                g_redis_global_data.uri_map.erase(iter++);
                continue;
            }
            flag = true;
        }
        ++iter;
    }

    if (!flag) {
        log("[error] a connection error|%p\n", c);
    }
}

void on_thread_redis_disconnect(const redisAsyncContext* c, int) {
    bool flag = false;
    for (auto iter = g_redis_global_data.uri_map.begin(); iter != g_redis_global_data.uri_map.end(); ) {
        if (iter->second->ctxt == c) {
            log("a disconnection|%p|uri:%s\n", c, iter->second->addr.uri.c_str());
            iter->second->state = enum_disconnected_state;
            // erase掉
            g_redis_global_data.uri_map.erase(iter++);
            flag = true;
            continue;
        }
        ++iter;
    }

    if (!flag) {
        log("[error] a disconnection error|%p\n", c);
    }
}

void on_thread_redis_selectdb(void*, void* r, void* user_data) {
    redis_custom_data* data = (redis_custom_data*)user_data;
    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        data->core->state = enum_selected_state;
    }
    else {
        data->core->state = enum_disconnected_state;
        log("[error] failed to select db|%d\n", reply->type);
    }

    // delete redis_custom_data;
    delete data;
}

void on_thread_redis_result(void* c, void* r, void* user_data) {
    // 将结果写入队列
    redis_respond_data rsp_data; 
    rsp_data.reply = copy_redis_reply((redisReply*)r);
    rsp_data.data = (redis_custom_data*)user_data;
    g_redis_global_data.respond_queue.push(rsp_data);
}

void on_thread_redis_timeout(redis_custom_data* data) {
    redisReply* r = (redisReply*)calloc(1, sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->type = REDIS_REPLY_SELF_TIMEOUT;
    redis_respond_data rsp_data; 
    rsp_data.reply = r;
    rsp_data.data = data;
    g_redis_global_data.respond_queue.push(rsp_data);
}

//////////////////////////////////////////////////////////////////////////////////////

redis_core_ptr thread_create_core(const redis_addr& addr, bool new_create) {
    auto iter = g_redis_global_data.uri_map.find(addr.uri);
    if (iter != g_redis_global_data.uri_map.end()) {
        if (new_create || iter->second->state == enum_disconnected_state) {
            /*
             new_create为true时，意味着连接应该是出现了断开现象，不确定
             其内部是否会进行断线重连，这里为了保险起见，会将连接抛弃掉
             重新创建一个
            */
            iter->second->state = enum_disconnected_state;
            g_redis_global_data.uri_map.erase(iter);
            iter = g_redis_global_data.uri_map.end();
        }
    }

    if (iter == g_redis_global_data.uri_map.end()) {
        time_t now = 0;
        time(&now);

        // 不允许连接快速创建多个相同连接
        time_t last_t = g_redis_global_data.uri_ct_map[addr.uri];
        if (last_t != 0 && (now - last_t <= 1)) {
            return nullptr;
        }

        // 创建一个
        redis_core_ptr core = std::make_shared<redis_core>();
        core->state = enum_connecting_state;
        core->addr = addr;
        if (core->addr.cluster) {
            core->ctxt = redisClusterAsyncConnect(addr.host.c_str(), HIRCLUSTER_FLAG_NULL);
            redisClusterLibeventAttach((redisClusterAsyncContext *)core->ctxt, g_redis_global_data.evt_base);
            redisClusterAsyncSetConnectCallback((redisClusterAsyncContext *)core->ctxt, on_thread_redis_connect);
            redisClusterAsyncSetDisconnectCallback((redisClusterAsyncContext *)core->ctxt, on_thread_redis_disconnect);
        }
        else {
            core->ctxt = redisAsyncConnect(addr.host.c_str(), addr.port);
            redisLibeventAttach((redisAsyncContext *)core->ctxt, g_redis_global_data.evt_base);
            redisAsyncSetConnectCallback((redisAsyncContext *)core->ctxt, on_thread_redis_connect);
            redisAsyncSetDisconnectCallback((redisAsyncContext *)core->ctxt, on_thread_redis_disconnect);   
        }

        if (!core->ctxt) {
            log("[error] failed to call redisClusterAsyncConnect or redisAsyncConnect\n");
            return nullptr;
        }

        g_redis_global_data.uri_map[addr.uri] = core;
        g_redis_global_data.uri_ct_map[addr.uri] = now;
        return nullptr;
    }

    if (iter->second->state == enum_selected_state) {
        return iter->second;
    }

    return nullptr;
}

void thread_redis_op(redis_custom_data* user_data, void(*fn)(void*, void*, void*)) {
    // 转换cmd
    size_t cmd_size = user_data->cmd.cmd.size();
    std::vector<const char *> argv(cmd_size);
    std::vector<size_t> argv_len(cmd_size);
    for (size_t idx = 0; idx < cmd_size; ++idx) {
        if (user_data->cmd.cmd[idx].empty()) {
            argv[idx] = 0;
        }
        else {
            argv[idx] = user_data->cmd.cmd[idx].c_str();
        }
        argv_len[idx] = user_data->cmd.cmd[idx].length();
    }

    if (user_data->core->addr.cluster) {
        redisClusterAsyncCommandArgv((redisClusterAsyncContext *)user_data->core->ctxt,
                                     (void (*)(redisClusterAsyncContext*, void*, void*))fn,
                                     (void *)user_data,
                                     cmd_size,
                                     &(argv[0]),
                                     &(argv_len[0]));
    }
    else {
        redisAsyncCommandArgv((redisAsyncContext *)user_data->core->ctxt,
                              (void (*)(redisAsyncContext*, void*, void*))fn,
                              (void *)user_data,
                              cmd_size,
                              &(argv[0]),
                              &(argv_len[0]));
    }
}

void thread_redis_state() {
    for (auto iter = g_redis_global_data.uri_map.begin(); iter != g_redis_global_data.uri_map.end(); ++iter) {
        if (iter->second->state == enum_connected_state) {
            // 连接成功
            iter->second->state = enum_selecting_state;
            // 构造一个请求
            redis_custom_data* data = new redis_custom_data;
            data->core = iter->second;
            data->cmd = SelectCmd(iter->second->addr.idx);
            // 执行redis请求, 选数据库
            thread_redis_op(data, on_thread_redis_selectdb);
        }
    }
}

void thread_redis_process() {
    time_t now = 0;
    time(&now);
    thread_redis_state();
    for (auto iter = g_redis_global_data.prepare_queue.begin(); iter != g_redis_global_data.prepare_queue.end();) {
        redis_custom_data* data = *iter;
        redis_core_ptr core = thread_create_core(data->addr, false);
        if (!core) {
            if (now - data->req_time > 30) {
                // 10秒连接不上就超时
                on_thread_redis_timeout(data);
                iter = g_redis_global_data.prepare_queue.erase(iter);
            }
            else {
                ++iter;
            }
            continue;
        } 
        data->core = core;
        iter = g_redis_global_data.prepare_queue.erase(iter);
        // 执行请求
        thread_redis_op(data, on_thread_redis_result);
    }
    event_base_loop(g_redis_global_data.evt_base, EVLOOP_NONBLOCK);
    g_redis_global_data.flag = false;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    g_redis_global_data.thr_func = f;
}

inline std::function<void()> get_thread_func() {
    g_redis_global_data.flag = true;
    return thread_redis_process;
}

//////////////////////////////////////////////////////////////////////////////////////

bool local_redis_init() {
    if (g_redis_global_data.init) {
        return true;
    }

    g_redis_global_data.evt_base = event_base_new();
    if (!g_redis_global_data.evt_base) {
        log("[error] failed to call event_base_new\n");
        return false;
    }

    g_redis_global_data.init = true;
    return true;
}

bool set_event_base(void* evt_base) {
    if (!g_redis_global_data.init) {
        return false;
    }

    g_redis_global_data.evt_base = (event_base*)evt_base;
    g_redis_global_data.init = true;
    return true;
}

void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb) {
    // 构造一个请求
    local_redis_init();
    redis_custom_data* data = new redis_custom_data;
    data->cb = cb;
    data->cmd = redis_cmd;
    data->addr.Parse(uri);
    time(&data->req_time);
    g_redis_global_data.request_queue.push(data);
}

void statistics(uint32_t cur_time) {
    if (!g_log_cb) {
        return;
    }

    if (cur_time - g_last_statistics_time <= 120) {
        return;
    }

    g_last_statistics_time = cur_time;
    log("[statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        (g_redis_global_data.req_task_cnt - g_redis_global_data.rsp_task_cnt),
        g_redis_global_data.req_task_cnt,
        g_redis_global_data.rsp_task_cnt);
    log("[statistics] cur_uri:%ld", g_redis_global_data.uri_map.size());
}

void local_process_respond() {
    if (g_redis_global_data.respond_queue.empty()) {
        return;
    }

    // 交换出结果队列
    std::queue<redis_respond_data> respond_queue;
    g_redis_global_data.respond_queue.swap(respond_queue);

    // 运行结果
    while (!respond_queue.empty()) {
        // 取出结果
        g_redis_global_data.rsp_task_cnt++;
        redis_respond_data rsp = std::move(respond_queue.front());
        respond_queue.pop();
        RedisReplyParserPtr parser = std::make_shared<RedisReplyParser>(rsp.reply);
        rsp.data->cb(parser);
        delete rsp.data;
    }
}

void local_process_request() {
    while (!g_redis_global_data.request_queue.empty()) {
        // 挪到准备队列中
        redis_custom_data* req = std::move(g_redis_global_data.request_queue.front());
        g_redis_global_data.request_queue.pop();
        g_redis_global_data.prepare_queue.push_back(req);
        g_redis_global_data.req_task_cnt++;
    }
}

void local_process_task() {
    // 执行
    if (g_redis_global_data.req_task_cnt != g_redis_global_data.rsp_task_cnt) {
        if (g_redis_global_data.thr_func) {
            g_redis_global_data.thr_func(get_thread_func());
        }
        else {
            get_thread_func()();
        }
    }
}

bool loop(uint32_t cur_time) {
    if (!g_redis_global_data.init) {
        return false;
    }

    statistics(cur_time);

    // flag为false时，代表可操作
    if (g_redis_global_data.flag) {
        return true;
    }

    local_process_respond();
    local_process_request();
    local_process_task();

    // 是否有任务
    bool has_task = false;    
    if (g_redis_global_data.req_task_cnt != g_redis_global_data.rsp_task_cnt) {
        has_task = true;
    }
    return has_task;
}

} // redis
} // async

#endif