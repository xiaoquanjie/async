/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace net {

// tcp监听者  
class TcpListener {
public:
    friend void on_accept_cb_helper(TcpListener*, void* sock, void* bev);
    friend void on_cb_send(void* bev, void* ctx);
    friend void on_cb_recv(void* bev, void* ctx);

    struct ConnInfo {
        void* buff_ev = 0;
        bool is_closing = false;    // 是否在关闭中
        int  s_data_cnt = 0;        // 发送字节数
        std::string ip;             // 对方ip
    };

    // 如果event_base传递空，则内部将自创建一个base
    TcpListener(void* event_base);

    virtual ~TcpListener();

    void* getEventBase();

    // @time_t保留
    void update(time_t);

    // @addr: ip:port
    bool listen(const std::string& addr);

    // 发送数据
    int send(uint32_t fd, const char* buf, uint32_t len);

    // 主动关闭连接
    void close(uint32_t fd);

    // 设置connected回调
    void setAcceptCb(std::function<void(TcpListener*, uint32_t)> cb);

    // 设置连接断开时的回调，无论什么原因，只要断开必然会触发
    void setClosedCb(std::function<void(TcpListener*, uint32_t)> cb);

    // 设置接受数据的回调: 一个完整包的数据(如果没有设置解析器，则返回捕捉到的每一段数据)
    void setDataCb(std::function<void(TcpListener*, uint32_t, const char*, uint32_t)> cb);

    // 设置包解析器:
    // @header_size: 头包长度
    // @parser: 返回值包体长度
    void setParser(uint32_t header_size, std::function<uint32_t(const char*, uint32_t)> parser);
protected:
    void closed(uint32_t fd);

protected:
    bool  m_created_base;
    void* m_event_base;
    void* m_listener;
    uint32_t m_header_size;
    std::unordered_map<uint32_t, ConnInfo> m_conn_map;
    std::function<void(TcpListener*, uint32_t)> m_connected_cb;
    std::function<void(TcpListener*, uint32_t)> m_disconnected_cb;
    std::function<void(TcpListener*, uint32_t, const char*, uint32_t)> m_data_cb;
    std::function<uint32_t(const char*, uint32_t)> m_parser;
    char* m_header_buf;
};

} 
