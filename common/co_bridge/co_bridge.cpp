#include "common/co_bridge/co_bridge.h"
#include "common/co_bridge/time_pool.h"
#include "common/async/async.h"
#include <unordered_set>
#include <assert.h>

namespace co_bridge { 

int64_t g_unique_id = 1;
std::unordered_set<int64_t> g_unique_id_set;
TimerPool g_time_pool;

int64_t gen_unique_id() {
    return g_unique_id++;
}

void add_unique_id(int64_t id) {
    assert(g_unique_id_set.find(id) == g_unique_id_set.end());
    g_unique_id_set.insert(id);
}

bool rm_unique_id(int64_t id) {
    auto iter = g_unique_id_set.find(id);
    if (iter != g_unique_id_set.end()) {
        g_unique_id_set.erase(iter);
        return true;
    }
    return false;
}

int64_t add_timer(int interval, std::function<void()> func) {
    return g_time_pool.AddTimer(interval, func);
}

void rm_timer(int64_t timer_id) {
    g_time_pool.CancelTimer(timer_id);
}

bool loop() {
    g_time_pool.Update();
    return async::loop();
}

}