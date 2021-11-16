/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "common/net/http_listener.h"
#include <event.h>
#include <evhttp.h>

namespace net {

void on_request_cb(evhttp_request*, void*);

HttpListener::HttpListener(void* event_base) {
    m_event_base = event_base;
    m_created_base = false;
    m_evhttp = nullptr;
    if (!event_base) {
        m_event_base = event_base_new();
        m_created_base = true;
    }
}

HttpListener::~HttpListener() {
    if (m_evhttp) {
        evhttp_free((evhttp*)m_evhttp);
    }
    if (m_created_base) {
        event_base_free((event_base*)m_event_base);
    }
}

void* HttpListener::getEventBase() {
    return m_event_base;
}

void HttpListener::update(time_t) {
    if (m_created_base) {
        event_base_loop((event_base*)m_event_base, EVLOOP_NONBLOCK);
    }
}

bool HttpListener::listen(const std::string& ip, uint32_t port) {
    if (m_evhttp) {
        return false;
    }

    m_evhttp = evhttp_new((event_base*)m_event_base);
    evhttp_set_gencb((evhttp*)m_evhttp, on_request_cb, this);

    auto ret = evhttp_bind_socket((evhttp*)m_evhttp, ip.c_str(), port);
    if (ret != 0) {
        evhttp_free((evhttp*)m_evhttp);
        m_evhttp = nullptr;
        return false;
    }

    return true;
}

void HttpListener::send(void* request, const char* buf, uint32_t len) {
    std::unordered_map<std::string, std::string> header;
    header["Content-Type"] = "text/html; charset=utf8";
    send(request, header, buf, len);
}

void HttpListener::send(void *request,
                        std::unordered_map<std::string, std::string> &header,
                        const char *buf,
                        uint32_t len) {
    for (auto& item : header) {
        evhttp_add_header(evhttp_request_get_output_headers((evhttp_request*)request), item.first.c_str(), item.second.c_str());
    }
    auto evbuf = evhttp_request_get_output_buffer((evhttp_request*)request);
	evbuffer_add(evbuf, buf, len);
	evhttp_send_reply((evhttp_request*)request, 200, "OK", nullptr);
}

void HttpListener::setDataCb(std::function<void(HttpListener*, void*, const char*, uint32_t)> cb) {
    m_data_cb = cb;
}

const char* HttpListener::getRemoteHost(const void* request) {
    return ((const evhttp_request*)request)->remote_host;
}

const char* HttpListener::getUrlPath(const void* request) {
    auto path = evhttp_uri_get_path(((const evhttp_request*)request)->uri_elems);
    if (path) {
        return path;
    }
    return "";
}

const char* HttpListener::getUrlParam(const void* request) {
    // 获取查询字符串, 并且对特殊字符进行解码
	auto query = evhttp_uri_get_query(((const evhttp_request*)request)->uri_elems);
	return query;
}

///////////////////////////////////////////////////////////

void on_request_cb_helper(void* r, void* ctx) {
    HttpListener* l = (HttpListener*)(ctx);
    if (!l->m_data_cb) {
        return;
    }

    auto evbuf = evhttp_request_get_input_buffer((evhttp_request*)r);
    auto len = evbuffer_get_length(evbuf);
    if (len > 0) {
		auto buf = new char[len];
		if ((size_t)evbuffer_copyout(evbuf, buf, len) == len) {
           l->m_data_cb(l, r, buf, len);
        }
        delete[] buf;
	}
    else {
        l->m_data_cb(l, r, 0, 0);
    }
}

void on_request_cb(evhttp_request* request, void* ctx) {
    on_request_cb_helper(request, ctx);
}

}

#endif