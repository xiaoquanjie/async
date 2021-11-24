/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <unordered_map>

namespace net {

// 会话信息保留10分钟
class UdpListener {
public:
    struct SessionInfo {
        void* addr;
        time_t active = 0;
        SessionInfo();
        ~SessionInfo();
    };

    friend void on_cb_recv(int fd, short, void *arg);

    // 如果event_base传递空，则内部将自创建一个base
    UdpListener(void* event_base);

    virtual ~UdpListener();

    virtual void update(time_t cur_time);

    // @addr: ip:port
    bool listen(const std::string& ip, uint16_t port);

    int send(uint32_t fd, const char* buf, uint32_t len);

    // 设置接受数据的回调: 一个完整包的数据(如果没有设置解析器，则返回捕捉到的每一段数据)
    void setDataCb(std::function<void(UdpListener*, uint32_t, const char*, uint32_t)> cb);

protected:
    void expire(time_t cur_time);

protected:
    bool  m_created_base;
    void* m_event_base;
    void* m_evt;
    std::function<void(UdpListener*, uint32_t, const char*, uint32_t)> m_data_cb;
    char* m_data_buf;
    std::unordered_map<uint32_t, SessionInfo> m_fd_addr_map;
    uint32_t m_cur_poll_key;
};

}