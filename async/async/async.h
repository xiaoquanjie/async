/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/**
 *              异步工作模型描述
 * 第一种：同步core，如：async_cpu、async_mongo、async_rabbitmq
 * 第二种：异步串行core，如：aysnc_mysql
 * 第三中：异步并行core，如: aysnc_curl、aysnc_redis、aysnc_zookeeper
 *
 *
 * 同步core:
    1、work_thread 投递一个请求，将请求带上work_thread的信息进行包装，然后请求被放入本地请求队列
    2、work_thread运行loop
        （1）localStatistics打印统计信息
		（2）localRespond回调回复
		（3）从本地队列中弹出一个请求，将请求包装成一个任务抛入io任务队列
    3、io线程池
        （1）从io任务队列中取出一个任务
        （2）处理任务：根据请求信息在全局core队列中取出一个合适的core
         (3) 如果core队列中没有合适的core，则创建一个新的core
         (4) core处理完请求拿到请求结果，将结果放入work_thread的回复队列
         (5) core失效则关闭掉，core有效，则放回全局core队列
    
*  异步串行core:
    1、维护一个全局的globaldata，globaldata有一张连接池hash表，连接池中的每个连接有一个待处理请求列表
    2、work_thread 投递一个请求，将请求带上work_thread的信息进行包装，然后请求被放入本地请求队列
    3、work_thread运行loop
        （1）localStatistics打印统计信息
        （2）localRespond回调回复
        （3）遍历本地队列
            1）处理请求超时的情况
            2）从valid core列表取出一个有效的core，且core并发处理任务没有超过1个，则将请求放入core的待处理队列
            3）没有有效的core，且core的数量没有超过supposeIothread限制，则创建一个新core放入conning core列表
        （4）利用原子操作抢占dispatchTask标识
            1）抢占成功
            2）遍历globaldata连接池
            3）将无效连接从valid中移除到conning中
            4）将有效连接从conning中移除到valid中
            5）将所有的core分成supposeIothread组，每组打包成一个任务放入io任务队列
    4、io线程池
        （1）从io任务队列取出一个任务
        （2）处理任务：
            1）处理core的连接状态
            2）处理core的待处理任务列表

   并行异步core:
    1、维护一个全局的globaldata，globaldata有一张连接池hash表，连接池中的每个连接有一个待处理请求列表
    2、work_thread 投递一个请求，将请求带上work_thread的信息进行包装，然后请求被放入本地请求队列
    3、work_thread运行loop
        （1）localStatistics打印统计信息
        （2）localRespond回调回复
        （3）遍历本地队列
            1）处理请求超时的情况
            2）从valid core列表取出一个有效的core，且core并发处理任务没有超过上限，则将请求放入core的待处理队列
            3）没有有效的core，且core的数量没有超过supposeIothread限制，则创建一个新core放入conning core列表
        （4）利用原子操作抢占dispatchTask标识
            1）抢占成功
            2）遍历globaldata连接池
            3）将无效连接从valid中移除到conning中
            4）将有效连接从conning中移除到valid中
            5）将所有的core分成supposeIothread组，每组打包成一个任务放入io任务队列
    4、io线程池
        （1）从io任务队列取出一个任务
        （2）处理任务：
            1）处理core的连接状态
            2）处理core的待处理任务列表
*/

#pragma once

#include <functional>
#include <initializer_list>
#include "async/async/redis/async_redis.h"
#include "async/async/mongo/async_mongo.h"
#include "async/async/curl/async_curl.h"
#include "async/async/cpu/async_cpu.h"
#include "async/async/mysql/async_mysql.h"

namespace async {

bool loop(uint32_t curTime = 0);

// 带有sleep功能，在没有任务时降低cpu使用率
void loopSleep(uint32_t curTime, uint32_t sleepMil = 0);

// 设置io线程
void setThreadFunc(std::function<void(std::function<void()>)> cb);

// 假设最大的io线程数量
uint32_t supposeIothread();

// 设置io线程数量
void setIoThread(uint32_t c);

// 返回的是毫秒
clock_t getMilClock();

///////////////////////////////////////////////////////////////////////////////////////////

typedef std::function<void(int64_t)> done_cb;
typedef std::function<void(int64_t)> next_cb;
typedef std::function<void(next_cb)> fn_cb;

void parallel(done_cb done, const std::initializer_list<fn_cb>& fns);

void series(done_cb done, const std::initializer_list<fn_cb>& fns);

void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

/*
*example

async::parallel([](int err) {
    std::cout << "done:" << err << std::endl;
},
{
    [](async::next_cb next) {
        async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
            std::cout << "body1:" << body.length() <<std::endl;
            next(0);
        });
    },
    [](async::next_cb next) {
        async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
            std::cout << "body2:" << body.length() <<std::endl;
            next(2);
        });
    }
});

*/

};



