/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET 

#include "common/net/tcp_listener.h"
#include "common/log.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <assert.h>
#include <vector>

namespace net {

void on_accept_cb(evconnlistener* lst, evutil_socket_t fd, sockaddr* sock, int socklen, void* ctx);

/////////////////////////////////////////////////////////////////////////////////////

TcpListener::TcpListener(void* event_base) {
    m_event_base = event_base;
    m_header_size = 0;
    m_header_buf = 0;
    m_listener = 0;
    m_created_base = false;
    if (!event_base) {
        m_event_base = event_base_new();
        assert(m_event_base);
        m_created_base = true;
    }
}

TcpListener::~TcpListener() {
    if (m_listener) {
        evconnlistener* l = (evconnlistener*)m_listener;
        evconnlistener_free(l);
    }
    if (m_header_buf) {
        free(m_header_buf);
    }
    if (m_created_base) {
        event_base_free((event_base*)m_event_base);
    }
}

void* TcpListener::getEventBase() {
    return m_event_base;
}

void TcpListener::update(time_t) {
    /**
     * 这里可优化，可保存一个closing队列，避免轮询所有的fd
    */
    // std::vector<uint32_t> closing_fd_vec;
    // for (auto iter = m_conn_map.begin(); iter != m_conn_map.end(); ++iter) {
    //     if (iter->second.is_closing && iter->second.s_data_cnt == 0) {
    //         closing_fd_vec.push_back(iter->first);
    //     }
    // }
    // for (auto fd : m_closing_list) {
    //     closed(fd);
    // }
    for (auto iter = m_closing_list.begin(); iter != m_closing_list.end();) {
        auto iter2 = m_conn_map.find(*iter);
        if (iter2 != m_conn_map.end()) {
            if (iter2->second.s_data_cnt == 0) {
                closed(*iter);
                iter = m_closing_list.erase(iter);
                continue;
            }
        }
        ++iter;
    }

    if (m_created_base) {
        event_base_loop((event_base*)m_event_base, EVLOOP_NONBLOCK);
    }
}

bool TcpListener::listen(const std::string& addr) {
    if (m_listener) {
        return false;
    }
    
    sockaddr staddr;
	int len = sizeof(staddr);
	auto ret = evutil_parse_sockaddr_port(addr.c_str(), &staddr, &len);
	if (ret != 0) {
		assert(false);
		return false;
	}

    m_listener = evconnlistener_new_bind((event_base*)m_event_base, on_accept_cb, this,
                                         LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, &staddr, len);

    if (m_listener == nullptr) {
		assert(false);
		return false;
	}

    log("[tcpnet] listen: %s", addr.c_str());
    return true;
}

int TcpListener::send(uint32_t fd, const char* buf, uint32_t len) {
    auto iter = m_conn_map.find(fd);
    if (iter == m_conn_map.end()) {
        return -1;
    }

    // 正在关闭中，不允许发送数据了
    if (iter->second.is_closing) {
        return -1;
    }

    int snd = bufferevent_write((bufferevent*)iter->second.buff_ev, buf, len);
    if (snd < 0) {
        return -1;
    }

    iter->second.s_data_cnt += snd;
    return 0;
}

void TcpListener::close(uint32_t fd) {
    auto iter = m_conn_map.find(fd);
	if (iter == m_conn_map.end()) {
		return;
	}

    iter->second.is_closing = true;
    m_closing_list.push_back(fd);
}

std::string TcpListener::getIp(uint32_t fd) {
    auto iter = m_conn_map.find(fd);
	if (iter == m_conn_map.end()) {
		return std::string("");
	}

    return iter->second.ip;
}

void TcpListener::closed(uint32_t fd) {
    auto iter = m_conn_map.find(fd);
	if (iter == m_conn_map.end()) {
		return;
	}

    bufferevent_free((bufferevent*)iter->second.buff_ev);
    m_conn_map.erase(iter);
    if (m_disconnected_cb) {
        m_disconnected_cb(this, fd);
    }
}

void TcpListener::setAcceptCb(std::function<void(TcpListener*, uint32_t fd)> cb) {
    m_connected_cb = cb;
}

void TcpListener::setClosedCb(std::function<void(TcpListener*, uint32_t fd)> cb) {
    m_disconnected_cb = cb;
}

void TcpListener::setDataCb(std::function<void(TcpListener*, uint32_t, const char*, uint32_t)> cb) {
    m_data_cb = cb;
}

void TcpListener::setParser(uint32_t header_size, std::function<uint32_t(const char*, uint32_t)> parser) {
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

/////////////////////////////////////////////////////////////////////////////////

void on_cb_send(void* bev, void* ctx) {
    TcpListener* l = (TcpListener*)ctx;
    auto fd = bufferevent_getfd((bufferevent*)bev);
    auto iter = l->m_conn_map.find(fd);
    if (iter == l->m_conn_map.end()) {
        return;
    }

    // 更新剩余缓存
    struct evbuffer *output = bufferevent_get_output((bufferevent*)bev);
    iter->second.s_data_cnt = evbuffer_get_length(output);
}

void on_cb_recv(void* bev, void* ctx) {
    TcpListener* l = (TcpListener*)ctx;
    uint32_t head_size = l->m_header_size;
    uint32_t fd = 0;

    if (!l->m_data_cb) {
        return;
    }
   
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
            evbuffer_copyout(evbuf, l->m_header_buf, head_size);
            body_len = l->m_parser(l->m_header_buf, head_size);
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
        if (fd == 0) {
            fd = bufferevent_getfd((bufferevent*)bev);
        }
        l->m_data_cb(l, fd, pack_buf, pack_len);
        free(pack_buf);
    }
}

void on_cb_event(bufferevent* bev, short what, void* ctx) {
    TcpListener* l = (TcpListener*)ctx;
    auto fd = bufferevent_getfd(bev);

    if (what & BEV_EVENT_EOF) {
        // 结束
        l->close(fd);
    }
    else if (what & BEV_EVENT_ERROR) {
        // 出错
        l->close(fd);
    }
}

void on_accept_cb_helper(TcpListener* l, void* sock, void* bev) {
    // 获取ip
	auto paddr = reinterpret_cast<sockaddr_in*>((sockaddr*)sock);
	char ip[32];
	evutil_inet_ntop(AF_INET, &paddr->sin_addr, ip, sizeof(ip));

    // 获取fd
	auto fd = bufferevent_getfd((bufferevent*)bev);

    // 添加至map中
    TcpListener::ConnInfo info;
    info.buff_ev = bev;
    info.ip = ip;
    l->m_conn_map[fd] = info;
    if (l->m_connected_cb) {
        l->m_connected_cb(l, fd);
    }
}

// 接受新连接的回调
void on_accept_cb(evconnlistener* /*lst*/, evutil_socket_t fd, sockaddr* sock, int /*socklen*/, void* ctx) {
    TcpListener* listener = (TcpListener*)ctx;
    auto base = (event_base*)listener->getEventBase();

    typedef void(*cb)(bufferevent*, void*);
	bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, (cb)on_cb_recv, (cb)on_cb_send, on_cb_event, ctx);
	bufferevent_enable(bev, EV_READ);
	bufferevent_enable(bev, EV_WRITE);

    on_accept_cb_helper(listener, sock, bev);
}

}


#endif