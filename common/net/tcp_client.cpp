/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "common/net/tcp_client.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <assert.h>

namespace net {

void on_cb_recv_t(void* bev, void* ctx) {
    TcpClient* t = (TcpClient*)ctx;
    uint32_t head_size = t->m_header_size;

    while (true) {
        auto evbuf = bufferevent_get_input((bufferevent*)bev);
        uint32_t len = evbuffer_get_length(evbuf);
        if (len <= 0) {
            return;
        }
        if (len < head_size) {
            return;
        }

        uint32_t body_len = 0;
        if (head_size != 0) {
            evbuffer_copyout(evbuf, t->m_header_buf, head_size);
            body_len = t->m_parser(t->m_header_buf, head_size);
        }

        uint32_t pack_len = head_size + body_len;
        uint32_t buf_len = evbuffer_get_length(evbuf);
        if (pack_len == 0) {
            pack_len = buf_len;
        }
        if (buf_len < pack_len) {
            return;
        }

        char* pack_buf = (char*)malloc(pack_len);
        evbuffer_remove(evbuf, pack_buf, pack_len);
        t->m_data_cb(t, pack_buf, pack_len);
        free(pack_buf);
    }
}

void on_cb_event_t(void*, short what, void* ctx) {
    TcpClient* t = reinterpret_cast<TcpClient*>(ctx);
	if (what & BEV_EVENT_EOF) {
		if (t->m_disconnected_cb) {
            t->m_disconnected_cb(t);
        }
	}
}

TcpClient::TcpClient(void* event_base) {
    m_event_base = event_base;
    m_buff_ev = 0;
    m_header_size = 0;
    m_header_buf = 0;
    m_created_base = false;
    if (!event_base) {
        m_event_base = event_base_new();
        assert(m_event_base);
        m_created_base = true;
    }
}

TcpClient::~TcpClient() {
    if (m_buff_ev) {
        bufferevent_free((bufferevent*)m_buff_ev);
    }
    if (m_header_buf) {
        free(m_header_buf);
    }
    if (m_created_base) {
        event_base_free((event_base*)m_event_base);
    }
}

void TcpClient::update(time_t) {
    if (m_created_base) {
        event_base_loop((event_base*)m_event_base, EVLOOP_NONBLOCK);
    }
}

bool TcpClient::connect(const std::string& addr) {
    auto fd = socket(AF_INET, SOCK_STREAM, 0);
	m_buff_ev = bufferevent_socket_new((event_base*)m_event_base, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_enable((bufferevent*)m_buff_ev, EV_READ | EV_PERSIST);

    typedef void(*cb)(bufferevent*, void*);
    typedef void(*cb2)(bufferevent*, short, void*);
	bufferevent_setcb((bufferevent*)m_buff_ev, (cb)on_cb_recv_t, NULL, (cb2)on_cb_event_t, this);

	sockaddr sock_addr;
	int len = sizeof(sockaddr_in);
	evutil_parse_sockaddr_port(addr.c_str(), &sock_addr, &len);
	return (bufferevent_socket_connect((bufferevent*)m_buff_ev, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == 0);
}

void TcpClient::send(const char* buf, uint32_t len) {
    bufferevent_write((bufferevent*)m_buff_ev, buf, len);
}

void TcpClient::setClosedCb(std::function<void(TcpClient*)> cb) {
    m_disconnected_cb = cb;
}

void TcpClient::setDataCb(std::function<void(TcpClient*, const char*, uint32_t)> cb) {
    m_data_cb = cb;
}

void TcpClient::setParser(uint32_t header_size, std::function<uint32_t(const char*, uint32_t)> parser) {
    if (header_size == 0 || !parser) {
        return;
    }

    m_header_size = header_size;
    m_parser = parser;

    if (m_header_buf) {
        free(m_header_buf);
    }
    m_header_buf = (char*)malloc(m_header_size);
}

}

#endif