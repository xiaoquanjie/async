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

void logZeroMsg(zmq_msg_t* msg, bool is_addr, const char* desc) {
    return;
    auto data_len = zmq_msg_size(msg);
    if (data_len > 0) {
        if (is_addr) {
            std::string data;
            void *d = zmq_msg_data(msg);
            data.assign(reinterpret_cast<char *>(d), data_len);
            printf("zero_addr:%s|%s\n", data.c_str(), desc);
        }
        else {
            printf("zero_msg:(len):%ld|%s\n", data_len, desc);
        }
    }
}

void copyFromZmqMsg(zmq_msg_t* msg, std::string& data) {
    auto data_len = zmq_msg_size(msg);
    void *d = zmq_msg_data(msg);
    data.assign(reinterpret_cast<char *>(d), data_len);
}

/////////////////////////////////////

ZeromqUnit::Context ZeromqUnit::m_ctx;

ZeromqUnit::Context::Context() {
    m_gen_id = 1;
	m_ctx = zmq_ctx_new();
	assert(m_ctx);
}

ZeromqUnit::Context::~Context() {
	zmq_ctx_term(m_ctx);
}

///////////////////////////////////////////

ZeromqUnit::ZeromqUnit(uint32_t identify, const std::string& addr) {
    m_zeromq_id = identify;
    m_unique_id = m_ctx.m_gen_id++;
    m_addr = addr;
    m_sock = 0;
}

ZeromqUnit::~ZeromqUnit() {
    if (m_sock) {
        zmq_close(m_sock);
        m_sock = 0;
    }
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

uint32_t ZeromqUnit::zeromqId() {
    return m_zeromq_id;
}

uint32_t ZeromqUnit::uniqueId() {
    return m_unique_id;
}

bool ZeromqUnit::reInit(bool is_router) {
    if (m_sock) {
        zmq_close(m_sock);
    }

    if (is_router) {
        m_sock = zmq_socket(m_ctx.m_ctx, ZMQ_ROUTER);
    }
    else {
        m_sock = zmq_socket(m_ctx.m_ctx, ZMQ_DEALER);
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

int ZeromqUnit::send(bool is_router, const std::string& identify, const std::string& data) {
    int len = 0;
    do {
        if (is_router) {
            // 路由
            zmq_msg_t addr_msg;
            copyZmqMsg(&addr_msg, identify);
            // 第一帧先发对端地址
            logZeroMsg(&addr_msg, true, "send");
            len = zmq_msg_send(&addr_msg, m_sock, ZMQ_SNDMORE);
            zmq_msg_close(&addr_msg);
            if (len == -1) {
                break;
            }
        }

        // 最后一帧发数据
        zmq_msg_t send_msg;
        copyZmqMsg(&send_msg, data);
        logZeroMsg(&send_msg, false, "send");
        len = zmq_msg_send(&send_msg, m_sock, 0);
        zmq_msg_close(&send_msg);
    } while (false);

    if (len == -1) {
        printf("failed to send data, data_size:%d, err:%s\n", (int)data.size(), zmq_strerror(errno));
    }

    return len; 
}

void ZeromqUnit::printError() {
    printf("zeromq error:%s\n", zmq_strerror(errno));
}

//////////////////////////////////////////////////////////

ZeromqRouter::ZeromqRouter(uint32_t identify, const std::string& addr) 
    : ZeromqUnit(identify, addr) {
}

bool ZeromqRouter::listen() {
    reInit(true);

    std::string id = identify();
    if(zmq_setsockopt(m_sock, ZMQ_IDENTITY, id.c_str(), id.length()) < 0) {
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

    printf("zeromq listen:%s|%s\n", m_addr.c_str(), id.c_str());
    return true;
}

int ZeromqRouter::recvData(std::string& identify, std::string& data) {
    do {
        // 第一帧是来源地址
        zmq_msg_t addr_msg;
        copyZmqMsg(&addr_msg, "");
        int len = zmq_msg_recv(&addr_msg, m_sock, ZMQ_DONTWAIT);
        logZeroMsg(&addr_msg, true, "recv");
        // 拷贝addr
        copyFromZmqMsg(&addr_msg, identify);
        zmq_msg_close(&addr_msg);

        if (len == -1) {
            if (errno != EAGAIN) {
                printError();
            }
            break;
        }

        // 最后一帧是数据
        zmq_msg_t recv_msg;
        copyZmqMsg(&recv_msg, "");
        len = zmq_msg_recv(&recv_msg, m_sock, ZMQ_DONTWAIT);
        logZeroMsg(&recv_msg, false, "recv");

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

std::string ZeromqRouter::identify() {
    std::string id = "listen_";
    id += std::to_string(m_zeromq_id);
    return id;
}

int ZeromqRouter::sendData(const std::string& identify, const std::string& data) {
    return this->send(true, identify, data);
}

int ZeromqRouter::sendData(uint32_t identify, const std::string& data) {
    std::string id = "connect_" + std::to_string(identify);
    return sendData(id, data);
}

//////////////////////////////////////////////////////////

ZeromqDealer::ZeromqDealer(uint32_t identify, const std::string& addr) 
    : ZeromqUnit(identify, addr) {
}

bool ZeromqDealer::connect() {
    reInit(false);

    std::string id = identify();
    if(zmq_setsockopt(m_sock, ZMQ_IDENTITY, id.c_str(), id.length()) < 0) {
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

    printf("zeromq connect:%s|%s\n", m_addr.c_str(), id.c_str());
    return true;
}

int ZeromqDealer::recvData(std::string&, std::string& data) {
    do {
        // 最后一帧是数据
        zmq_msg_t recv_msg;
        copyZmqMsg(&recv_msg, "");
        int len = zmq_msg_recv(&recv_msg, m_sock, ZMQ_DONTWAIT);
        logZeroMsg(&recv_msg, false, "recv");

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

std::string ZeromqDealer::identify() {
    std::string id = "connect_";
    id += std::to_string(m_zeromq_id);
    return id;
}

int ZeromqDealer::sendData(const std::string& data) {
    return this->send(false, "", data);
}

#endif