/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_IPC

#include "common/ipc/zero_mq_unit.h"
#include <zmq.h>
#include <assert.h>
#include <string.h>

void copyZmqMsg(zmq_msg_t* msg, const std::string& data) {
    zmq_msg_init(msg);
    if (!data.empty()) {
        int len = data.size();
	    zmq_msg_init_size(msg, len);
        memcpy(zmq_msg_data(msg), data.c_str(), len);
    }
}

/////////////////////////////////////

ZeromqUnit::Context ZeromqUnit::m_ctx;

ZeromqUnit::Context::Context() {
	m_ctx = zmq_ctx_new();
	assert(m_ctx);
}

ZeromqUnit::Context::~Context() {
	zmq_ctx_term(m_ctx);
}

///////////////////////////////////////////

ZeromqUnit::ZeromqUnit(uint32_t id, const std::string& addr) {
    m_id = id;
    m_addr = addr;
    m_sock = 0;
    m_is_conn = false;
}

ZeromqUnit::~ZeromqUnit() {
    if (m_sock) {
        zmq_close(m_sock);
        m_sock = 0;
    }
}

bool ZeromqUnit::listen() {
    reInit();

    std::string identify = "listen_";
    identify += std::to_string(m_id);

    if(zmq_setsockopt(m_sock, ZMQ_IDENTITY, identify.c_str(), identify.length()) < 0) {
        printError();
        assert(false);
        return false;
    }

    auto ret = zmq_bind(m_sock, m_addr.c_str());
	if (ret != 0) {
		printError();
		assert(false);
		return false;
	}

    printf("zeromq listen:%s|%s\n", m_addr.c_str(), identify.c_str());
    return true;
}

bool ZeromqUnit::connect() {
    m_is_conn = true;
    reInit();

    std::string identify = "connect_";
    identify += std::to_string(m_id);

    if(zmq_setsockopt(m_sock, ZMQ_IDENTITY, identify.c_str(), identify.length()) < 0) {
        printError();
        assert(false);
        return false;
    }

    auto ret = zmq_connect(m_sock, m_addr.c_str());
	if (ret != 0) {
		printError();
		assert(false);
		return false;
	}

    printf("zeromq connect:%s|%s\n", m_addr.c_str(), identify.c_str());
    return true;
}

int ZeromqUnit::recvFd() {
    int val = 0;
	if (m_sock) {
		auto valen = sizeof(val);
		zmq_getsockopt(m_sock, ZMQ_FD, &val, &valen);
	}
	return val;
}

void* ZeromqUnit::getSock() {
    return m_sock;
}

bool ZeromqUnit::isPollIn() {
    if (m_sock) {
		int val = 0;
		auto valen = sizeof(val);
		zmq_getsockopt(m_sock, ZMQ_EVENTS, &val, &valen);
		return val & ZMQ_POLLIN;
	}

	return false;    
}

int ZeromqUnit::recvData(std::string& data) {
    do {
        if (!m_is_conn) {
            // 第一帧是来源地址
            zmq_msg_t addr_msg;
            copyZmqMsg(&addr_msg, "");
            int len = zmq_msg_recv(&addr_msg, m_sock, ZMQ_DONTWAIT);
            zmq_msg_close(&addr_msg);

            if (len == -1) {
                if (errno != EAGAIN) {
                    printError();
                }
                break;
            }
        }

        // 最后一帧是数据
        zmq_msg_t recv_msg;
        copyZmqMsg(&recv_msg, "");
        int len = zmq_msg_recv(&recv_msg, m_sock, ZMQ_DONTWAIT);

        if (len == -1) {
            if (errno != EAGAIN) {
                printError();
            }
            zmq_msg_close(&recv_msg);
        }
        else {
            auto data_len = zmq_msg_size(&recv_msg);
            if (data_len > 0) {
                void *d = zmq_msg_data(&recv_msg);
                data.assign(reinterpret_cast<char *>(d), data_len);
            }

            zmq_msg_close(&recv_msg);
            return data_len;
        }
    } while (false);
    
    return 0;
}

int ZeromqUnit::sendData(uint32_t id, const std::string& data) {
    int len = 0;
    do {
        if (!m_is_conn) {
            // 路由
            std::string identify = "connect_" + std::to_string(id);
            zmq_msg_t addr_msg;
            copyZmqMsg(&addr_msg, identify);
            // 第一帧先发对端地址
            len = zmq_msg_send(&addr_msg, m_sock, ZMQ_SNDMORE);
            zmq_msg_close(&addr_msg);
            if (len == -1) {
                break;
            }
        }

        // 最后一帧发数据
        zmq_msg_t send_msg;
        copyZmqMsg(&send_msg, data);
        len = zmq_msg_send(&send_msg, m_sock, 0);
        zmq_msg_close(&send_msg);
    } while (false);

    if (len == -1) {
        printf("failed to send data, data_size:%d, err:%s\n", (int)data.size(), zmq_strerror(errno));
    }

    return len; 
}

int ZeromqUnit::sendData(const std::string& data) {
    return sendData(0, data);
}

uint32_t ZeromqUnit::id() {
    return m_id;
}

bool ZeromqUnit::reInit() {
    if (m_sock) {
        zmq_close(m_sock);
    }

    if (m_is_conn) {
        m_sock = zmq_socket(m_ctx.m_ctx, ZMQ_DEALER);
    }
    else {
        m_sock = zmq_socket(m_ctx.m_ctx, ZMQ_ROUTER);
    }

    if (!m_sock) {
        printError();
        assert(false);
        return false;
    }

    int32_t limit_hwm = 20000;
    zmq_setsockopt(m_sock, ZMQ_SNDHWM, &limit_hwm, sizeof(limit_hwm));
    zmq_setsockopt(m_sock, ZMQ_RCVHWM, &limit_hwm, sizeof(limit_hwm));
    return true;
}

void ZeromqUnit::printError() {
    printf("zeromq error:%s\n", zmq_strerror(errno));
}


#endif