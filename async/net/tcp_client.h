/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <functional>

namespace net {

class TcpClient {
public:
    friend void on_cb_recv_t(void* bev, void* ctx);
    friend void on_cb_event_t(void* bev, short what, void* ctx);

    // 如果event_base传递空，则内部将自创建一个base
    TcpClient(void* event_base);

    virtual ~TcpClient();

    void update(time_t);

    // @addr: ip:port
    bool connect(const std::string& addr);

    void send(const char* buf, uint32_t len);

    // 主动关闭连接
    void close();

    void setClosedCb(std::function<void(TcpClient*)> cb);

    // 设置接受数据的回调: 一个完整包的数据(如果没有设置解析器，则返回捕捉到的每一段数据)
    void setDataCb(std::function<void(TcpClient*, const char*, uint32_t)> cb);

    // 设置包解析器:
    // @header_size: 头包长度
    // @parser: 返回值包体长度
    void setParser(uint32_t header_size, std::function<uint32_t(const char*, uint32_t)> parser);
    
protected:
    bool  m_created_base;
    void* m_event_base;
    void* m_buff_ev;
    uint32_t m_header_size;
    char* m_header_buf;
    std::function<void(TcpClient*)> m_disconnected_cb;
    std::function<void(TcpClient*, const char*, uint32_t)> m_data_cb;
    std::function<uint32_t(const char*, uint32_t)> m_parser;
};

}