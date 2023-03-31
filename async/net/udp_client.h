/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace net {

class UdpClient {
public:
    friend void on_cb_recv_c(int fd, short, void *arg);

    UdpClient(void* event_base);

    virtual ~UdpClient();

    void update(time_t cur_time);

    bool connect(const std::string& addr);

    int send(const char* buf, uint32_t len);

    void setDataCb(std::function<void(UdpClient*, const char*, uint32_t)> cb);

protected:
    bool  m_created_base;
    void* m_event_base;
    void* m_evt;
    void* m_addr;
    char* m_data_buf;
    int32_t m_fd;
    std::function<void(UdpClient*, const char*, uint32_t)> m_data_cb;
};

}