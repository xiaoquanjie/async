/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET 

#include "common/net/udp_listener.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

namespace net {

void on_cb_recv(int fd, short what, void *arg) {
    UdpListener* l = (UdpListener*)arg;
    if (what | EV_READ) {
        if (!l->m_data_buf) {
            return;
        }
        auto& sinfo = l->m_fd_addr_map[fd];
		socklen_t len = sizeof(*((sockaddr*)sinfo.addr));
		auto ret = recvfrom(fd, l->m_data_buf, 1024, 0, (sockaddr*)sinfo.addr, &len);
        if (ret > 0 && l->m_data_cb) {
            time(&sinfo.active);
            l->m_data_cb(l, fd, l->m_data_buf, ret);
        }
    }
}

UdpListener::SessionInfo::SessionInfo() {
    addr = malloc(sizeof(sockaddr));
}

UdpListener::SessionInfo::~SessionInfo() {
    free(addr);
}

UdpListener::UdpListener(void* event_base) {
    m_created_base = 0;
    m_event_base = event_base;
    m_evt = 0;
    m_data_buf = 0;
    m_cur_poll_key = 0;
    if (!event_base) {
        m_event_base = event_base_new();
        assert(m_event_base);
        m_created_base = true;
    }
}

UdpListener::~UdpListener() {
    if (m_evt) {
        event_free((event*)m_evt);
    }
    if (m_created_base) {
        event_base_free((event_base*)m_event_base);
    }
    if (m_data_buf) {
        free(m_data_buf);
    }
}

void UdpListener::update(time_t cur_time) {
    if (m_created_base) {
        event_base_loop((event_base*)m_event_base, EVLOOP_NONBLOCK);
    }
    
    expire(cur_time);
}

void UdpListener::expire(time_t cur_time) {
    if (cur_time == 0) {
        return;
    }
    if (m_cur_poll_key == 0) {
        if (m_fd_addr_map.empty()) {
            return;
        }
        m_cur_poll_key = m_fd_addr_map.begin()->first;
    }

    int iterator_cnt = 0;
    auto iter = m_fd_addr_map.find(m_cur_poll_key);

    while (iter != m_fd_addr_map.end() && iterator_cnt <= 50) {
        iterator_cnt++;
        // 超过10分钟无访问,则删除
        if ((cur_time - iter->second.active) >= 10 * 60) {
            m_fd_addr_map.erase(iter++);
        }
        else {
            iter++;
        }
    }

    if (iter != m_fd_addr_map.end()) {
        m_cur_poll_key = iter->first;
    }
    else {
        m_cur_poll_key = 0;
    }
}

bool UdpListener::listen(const std::string& ip, uint16_t port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
       return false;
    }

    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
       close(fd);
       return false;
    }

    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    sock_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr)) < 0) {
       close(fd);
       return false;
    } 

    auto evt = event_new((event_base*)m_event_base, fd, EV_READ|EV_PERSIST, on_cb_recv, this);
    event_add(evt, nullptr);
    return true;
}

int UdpListener::send(uint32_t fd, const char* buf, uint32_t len) {
    auto iter = m_fd_addr_map.find(fd);
    if (iter == m_fd_addr_map.end()) {
        return -1;
    }

    sockaddr* sock_addr = (sockaddr*)iter->second.addr;
    return sendto(fd, buf, len, 0, sock_addr, sizeof(*sock_addr));
}

void UdpListener::setDataCb(std::function<void(UdpListener*, uint32_t, const char*, uint32_t)> cb) {
    m_data_cb = cb;
    if (!m_data_buf) {
        m_data_buf = (char*)malloc(1024);
    }
}

}

#endif