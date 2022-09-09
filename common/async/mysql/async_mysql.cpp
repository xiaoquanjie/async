/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/async_mysql.h"
#include "common/log.h"
#include <vector>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <mysql/errmsg.h>
#include <assert.h>
#include <list>
#include <mutex>
#include <map>
#include <memory>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mysql {

// mysql地址结构
struct mysql_addr {
    mysql_addr (const std::string& uri) {
        Parse(uri);
    }

    mysql_addr () {}

    void Parse(const std::string& uri) {
        this->uri = uri;
        std::vector<std::string> values;
        async::split(uri, "|", values);
        if (values.size() == 5) {
            this->host = values[0];
            this->port = atoi(values[1].c_str());
            this->db = values[2];
            this->user = values[3];
            this->pwd = values[4];
        }
        else {
            log("[async_mysql] [error] uri error:%s", uri.c_str());
            assert(false);
        }
    }

    std::string uri;
    std::string host;
    unsigned int port = 0;
    std::string db;
    std::string user;
    std::string pwd;
};

// mysql连接状态
enum {
    enum_null_state = 0,
    enum_connecting_state,
    enum_connected_state,
    enum_querying_state,
    enum_queryed_state,
    enum_storing_state,
    enum_stored_state,
    enum_error_state,
    enum_disconnected_state,
};

// mysql连接核心
struct mysql_core {
    MYSQL mysql;
    mysql_addr addr;
    int state = enum_null_state;
    int fd = 0;

    mysql_core() {}

    ~mysql_core() {
        mysql_close(&mysql);
    }
};

typedef std::shared_ptr<mysql_core> mysql_core_ptr;

struct mysql_custom_data {
    mysql_core_ptr core;
    async_mysql_cb cb;
    bool is_query = false;
    mysql_addr addr;
    std::string sql;
    int status = 0;
};

typedef std::shared_ptr<mysql_custom_data> mysql_custom_data_ptr;

struct mysql_respond_data {
    mysql_custom_data_ptr data = 0;
    MYSQL_RES* res = 0;
};

struct uri_data {
    std::string uri;
    unsigned int conns = 0;
    std::list<mysql_core_ptr> core_list;
};

struct mysql_global_data {
    std::function<void(std::function<void()>)> thr_func;
    unsigned int max_connections = 128;
    unsigned int keep_connections = 64;
    std::list<mysql_custom_data_ptr> prepare_queue;
    std::list<mysql_custom_data_ptr> request_queue;
    std::list<mysql_respond_data> respond_queue;
    std::mutex request_mutext;
    std::mutex respond_mutext;
    std::map<std::string, uri_data> uri_map;
    time_t last_check_time = 0;
    uint64_t req_task_cnt = 0;
    uint64_t rsp_task_cnt = 0;
};

mysql_global_data g_mysql_global_data;

time_t g_last_statistics_time = 0;

///////////////////////////////////////////////////////////////////////////////

mysql_core_ptr local_create_core(const mysql_addr& addr, uri_data& uri_map) {
    if (!uri_map.core_list.empty()) {
        auto core = uri_map.core_list.front();
        uri_map.core_list.pop_front();
        return core;
    }

    if (uri_map.conns >= g_mysql_global_data.max_connections) {
        return nullptr;
    }

    mysql_core_ptr core = std::make_shared<mysql_core>();
    core->addr = addr;
    core->state = enum_null_state;
    auto mysql = mysql_init(&core->mysql);
    if (!mysql) {
        log("[async_mysql] [error] failed to call mysql_init");
        return nullptr;
    }

    uri_map.conns++;
    if (uri_map.uri.empty()) {
        uri_map.uri = addr.uri;
    }
    mysql_options(&core->mysql, MYSQL_OPT_NONBLOCK, 0);
    return core;
}

mysql_custom_data_ptr local_create_mysql_custom_data(const std::string &uri,
                                                     const std::string &sql,
                                                     async_mysql_cb cb,
                                                     bool is_query)
{
    mysql_custom_data_ptr req = std::make_shared<mysql_custom_data>();
    req->addr.Parse(uri);
    req->sql = sql;
    req->cb = cb;
    req->is_query = is_query;
    
    g_mysql_global_data.prepare_queue.push_back(req);
    g_mysql_global_data.req_task_cnt++;
    return req;
}

void thread_mysql_look_state(std::list<mysql_custom_data_ptr> &request_queue,
                             std::list<mysql_respond_data> &respond_queue,
                             fd_set &fd_r,
                             fd_set &fd_w,
                             fd_set &fd_e,
                             int &max_fd)
{
    FD_ZERO(&fd_r);
    FD_ZERO(&fd_w);
    FD_ZERO(&fd_e);
    max_fd = 0;

    for (auto iter = request_queue.begin(); iter != request_queue.end();) {
        mysql_custom_data_ptr data = *iter;
        if (data->core->state == enum_null_state) {
            MYSQL* ret = 0;
            data->status = mysql_real_connect_start(&ret,
                                                  &data->core->mysql,
                                                  data->core->addr.host.c_str(),
                                                  data->core->addr.user.c_str(),
                                                  data->core->addr.pwd.c_str(),
                                                  data->core->addr.db.c_str(),
                                                  data->core->addr.port,
                                                  0,
                                                  0);
            if (data->status == 0) {
                data->core->state = enum_connected_state;
            }
            else {
                data->core->state = enum_connecting_state;
            }
            data->core->fd = mysql_get_socket(&data->core->mysql);
        }
        
        if (data->core->state == enum_connected_state) {
            int err = 0;
            data->status = mysql_real_query_start(&err,
                                                &data->core->mysql,
                                                data->sql.c_str(),
                                                data->sql.length());
            if (err != 0) {
                data->core->state = enum_error_state;
                log("[async_mysql] [error] failed to call mysql_real_query_start:%d", err);
            }
            else {
                if (data->status == 0) {
                    data->core->state = enum_queryed_state;
                }
                else {
                    data->core->state = enum_querying_state;
                }
            }
        }

        if (data->core->state == enum_queryed_state) {
            MYSQL_RES* res = 0;
            data->status = mysql_store_result_start(&res, &data->core->mysql);
            if (data->status == 0) {
                data->core->state = enum_stored_state;
                mysql_respond_data rsp;
                rsp.res = res;
                rsp.data = data;
                respond_queue.push_back(rsp);
                iter = request_queue.erase(iter);
                continue;
            }
            else {
                data->core->state = enum_storing_state;
            }
        }

        // 状态设置
        if (data->core->state == enum_connecting_state
            || data->core->state == enum_querying_state
            || data->core->state == enum_storing_state) {
            if (data->status & MYSQL_WAIT_READ) {
                FD_SET(data->core->fd, &fd_r);
            }
            if (data->status & MYSQL_WAIT_WRITE) {
                FD_SET(data->core->fd, &fd_w);
            }
            if (data->status & MYSQL_WAIT_EXCEPT) {
                FD_SET(data->core->fd, &fd_e);
            }
            if (data->core->fd > max_fd) {
                max_fd = data->core->fd;
            }
        }

        ++iter;
    }
}

void thread_mysql_wait(std::list<mysql_custom_data_ptr> &request_queue,
                       std::list<mysql_respond_data> &respond_queue,
                       fd_set &fd_r,
                       fd_set &fd_w,
                       fd_set &fd_e,
                       int &max_fd)
{
    if (max_fd == 0) {
        return;
    }

    struct timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    int rc = select(max_fd + 1, &fd_r, &fd_w, &fd_e, &to);

    if (rc < 0) {
        log("[async_mysql] [error] failed to call select");
        return;
    }

    if (rc == 0) {
        return;
    }

    for (auto iter = request_queue.begin(); iter != request_queue.end();) {
        mysql_custom_data_ptr data = *iter;
        int status = 0;
        
        if (FD_ISSET(data->core->fd, &fd_r)) {
            status |= MYSQL_WAIT_READ;
        }
        if (FD_ISSET(data->core->fd, &fd_w)) {
            status |= MYSQL_WAIT_WRITE;
        }
        if (FD_ISSET(data->core->fd, &fd_e)) {
            status |= MYSQL_WAIT_EXCEPT;
        }

        if (data->core->state == enum_connecting_state) {
            MYSQL* ret = 0;
            data->status = mysql_real_connect_cont(&ret, &data->core->mysql, status);
            if (data->status == 0) {
                if (!ret) {
                    data->core->state = enum_error_state;
                }
                else {
                    data->core->state = enum_connected_state;
                }
            }
        }

        if (data->core->state == enum_querying_state) {
            int err = 0;
            data->status = mysql_real_query_cont(&err, &data->core->mysql, status);
            if (data->status == 0) {
                if (err) {
                    data->core->state = enum_error_state;
                }
                else {
                    data->core->state = enum_queryed_state;
                }
            }
        }

        if (data->core->state == enum_storing_state) {
            MYSQL_RES* res = 0;
            data->status = mysql_store_result_cont(&res, &data->core->mysql, status);
            if (data->status == 0) {
                data->core->state = enum_stored_state;
                mysql_respond_data rsp;
                rsp.res = res;
                rsp.data = data;
                respond_queue.push_back(rsp);
                iter = request_queue.erase(iter);
                continue;
            }
        }

        if (data->core->state == enum_error_state) {
            mysql_respond_data rsp;
            rsp.res = 0;
            rsp.data = data;
            respond_queue.push_back(rsp);
            iter = request_queue.erase(iter);
            continue;
        }

        ++iter;
    }
}

void thread_mysql_process(std::list<mysql_custom_data_ptr> *request_queue, mysql_global_data* global_data) {
    std::list<mysql_respond_data> respond_queue;

    fd_set fd_r;
    fd_set fd_w;
    fd_set fd_e;
    int max_fd = 0;
    thread_mysql_look_state(*request_queue, respond_queue, fd_r, fd_w, fd_e, max_fd);
    thread_mysql_wait(*request_queue, respond_queue, fd_r, fd_w, fd_e, max_fd);

    // 处理结果
    {
        global_data->request_mutext.lock();
        global_data->request_queue.insert(global_data->request_queue.end(), request_queue->begin(), request_queue->end());
        global_data->request_mutext.unlock();
    }
    {
        global_data->respond_mutext.lock();
        global_data->respond_queue.insert(global_data->respond_queue.begin(), respond_queue.begin(), respond_queue.end());
        global_data->respond_mutext.unlock();
    }

    delete request_queue;
}

inline std::function<void()> get_thread_func() {
    // 取64个队列为一组
    std::list<mysql_custom_data_ptr>* request_queue = new std::list<mysql_custom_data_ptr>;

    {
        g_mysql_global_data.request_mutext.lock();
        auto e_iter = g_mysql_global_data.request_queue.begin();
        auto b_iter = e_iter;
        int count = 64;

        while (true)
        {
            if (count == 0)
            {
                break;
            }
            if (e_iter == g_mysql_global_data.request_queue.end())
            {
                break;
            }

            e_iter++;
            count--;
        }

        request_queue->insert(request_queue->end(), b_iter, e_iter);
        g_mysql_global_data.request_queue.erase(b_iter, e_iter);
        g_mysql_global_data.request_mutext.unlock();
    }

    assert(request_queue->size() != 0);
    auto f = std::bind(thread_mysql_process, request_queue, &g_mysql_global_data);
    return f;
}

///////////////////////////////////////////////////////////////////////////////

void query(const std::string& uri, const std::string& sql, async_mysql_cb cb) {
    local_create_mysql_custom_data(uri, sql, cb, true);
}

void execute(const std::string& uri, const std::string& sql, async_mysql_cb cb) {
    local_create_mysql_custom_data(uri, sql, cb, false);
}

void setMaxConnection(unsigned int cnt) {
    if (cnt != 0) {
        g_mysql_global_data.max_connections = cnt;
    }
}

void setKeepConnection(unsigned int cnt) {
    if (cnt != 0) { 
        g_mysql_global_data.keep_connections = cnt;
    }
}

void statistics(uint32_t cur_time) {
    if (cur_time - g_last_statistics_time <= 120) {
        return;
    }

    // 没有输出连接池大小
    g_last_statistics_time = cur_time;
    log("[async_mysql] [statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        (g_mysql_global_data.req_task_cnt - g_mysql_global_data.rsp_task_cnt),
        g_mysql_global_data.req_task_cnt,
        g_mysql_global_data.rsp_task_cnt);
    log("[async_mysql] [statistics] cur_uri:%ld", g_mysql_global_data.uri_map.size());
}

bool loop(uint32_t cur_time) {
    bool has_task = false;
    
    statistics(cur_time);

    // 创建
    std::list<mysql_custom_data_ptr> request_queue;
    for (auto iter = g_mysql_global_data.prepare_queue.begin(); iter != g_mysql_global_data.prepare_queue.end();) {
        mysql_custom_data_ptr data = *iter;
        uri_data& uri_map = g_mysql_global_data.uri_map[data->addr.uri];
        mysql_core_ptr core = local_create_core(data->addr, uri_map);
        if (!core) {
            ++iter;
            continue;
        }
        else {
            data->core = core;
            request_queue.push_back(data);
        }
        iter = g_mysql_global_data.prepare_queue.erase(iter);
    }

    {
        // 插入队例
        g_mysql_global_data.request_mutext.lock();
        g_mysql_global_data.request_queue.insert(g_mysql_global_data.request_queue.end(),
                                                 request_queue.begin(),
                                                 request_queue.end());
        g_mysql_global_data.request_mutext.unlock();
    }

    if (g_mysql_global_data.req_task_cnt != g_mysql_global_data.rsp_task_cnt) {
        has_task = true;
        if (!g_mysql_global_data.request_queue.empty()) {
            if (g_mysql_global_data.thr_func) {
                g_mysql_global_data.thr_func(get_thread_func());
            }
            else {
                get_thread_func()();
            }
        }
    }

    if (!g_mysql_global_data.respond_queue.empty()) {
        std::list<mysql_respond_data> respond_queue;
        g_mysql_global_data.respond_mutext.lock();
        respond_queue.swap(g_mysql_global_data.respond_queue);
        g_mysql_global_data.respond_mutext.unlock();

        // 运行结果
        for (auto& item : respond_queue) {
            g_mysql_global_data.rsp_task_cnt++;
            uri_data& uri_map = g_mysql_global_data.uri_map[item.data->addr.uri];
            
            int err = mysql_errno(&item.data->core->mysql);
            if (err != 0) {
                log("[async_mysql] [error] failed to call mysql:%s|%s", mysql_error(&item.data->core->mysql), item.data->sql.c_str());
            }

            int affected_rows = 0;
            if (!item.data->is_query) {
                affected_rows = (int)mysql_affected_rows(&item.data->core->mysql);
            }
            
            // 执行回调
            auto reply = std::make_shared<MysqlReplyParser>(item.res, affected_rows, err);
            item.data->cb(reply);

            // 归还连接
            if (err == CR_SERVER_GONE_ERROR 
                || CR_SERVER_LOST == err 
                || CR_CONN_HOST_ERROR == err 
                || CR_CONNECTION_ERROR == err
                || CR_UNKNOWN_HOST == err) {
                uri_map.conns--;
            }
            else {
                item.data->core->state = enum_connected_state;
                uri_map.core_list.push_back(item.data->core);
            }
        }
    }

    return has_task;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    g_mysql_global_data.thr_func = f;
}

}    
}

#endif