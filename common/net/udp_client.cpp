/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET 

#include "common/net/udp_client.h"
#include <event2/event.h>
#include <assert.h>

namespace net {

void on_cb_recv_c(int fd, short what, void *arg) {
    UdpClient* u = (UdpClient*)arg;
    if (what | EV_READ) {
        if (!u->m_data_buf) {
            return;
        }
       
		socklen_t len = sizeof(*((sockaddr*)u->m_addr));
		auto ret = recvfrom(fd, u->m_data_buf, 1024, 0, (sockaddr*)u->m_addr, &len);
        if (ret > 0 && u->m_data_cb) {
            u->m_data_cb(u, u->m_data_buf, ret);
        }
    }
}

UdpClient::UdpClient(void* event_base) {
    m_created_base = 0;
    m_event_base = event_base;
    m_fd = 0;
    m_evt = 0;
    m_data_buf = 0;
    if (!event_base) {
        m_event_base = event_base_new();
        assert(m_event_base);
        m_created_base = true;
    }
    m_addr = malloc(sizeof(sockaddr));
}

UdpClient::~UdpClient() {
    if (m_fd != 0) {
        evutil_closesocket(m_fd);
    }
    if (m_evt) {
        event_free((event*)m_evt);
    }
    if (m_created_base) {
        event_base_free((event_base*)m_event_base);
    }
    free(m_addr);

    if (m_data_buf) {
        free(m_data_buf);
    }
}

void UdpClient::update(time_t) {
    if (m_created_base) {
        event_base_loop((event_base*)m_event_base, EVLOOP_NONBLOCK);
    }
}

bool UdpClient::connect(const std::string& addr) {
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
       return false;
    }

    sockaddr* sock_addr = (sockaddr*)m_addr;
    int len = sizeof(*sock_addr);
    if (evutil_parse_sockaddr_port(addr.c_str(), sock_addr, &len) != 0) {
        evutil_closesocket(m_fd);
        return false;
    }

    if (::connect(m_fd, sock_addr, len) != 0) {
        return false;
    }

    m_evt = event_new((event_base*)m_event_base, m_fd, EV_READ|EV_PERSIST, on_cb_recv_c, this);
    event_add((event*)m_evt, nullptr);
    return true;
}

int UdpClient::send(const char* buf, uint32_t len) {
    sockaddr* sock_addr = (sockaddr*)m_addr;
    int addr_len = sizeof(*sock_addr);
    return sendto(m_fd, buf, len, 0, sock_addr, addr_len);
}

void UdpClient::setDataCb(std::function<void(UdpClient*, const char*, uint32_t)> cb) {
    m_data_cb = cb;
    if (!m_data_buf) {
        m_data_buf = (char*)malloc(1024);
    }
}

}

#endif