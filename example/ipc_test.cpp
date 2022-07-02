#ifdef USE_IPC

#include "./common.h"

struct IpcHeader {
    int cmd = 0;
    uint64_t seqId = 0;
    int size = 0;
};

struct IpcPacket {
    char buf[1024] = { 0 };
    IpcHeader* getHeader () {
        IpcHeader* h = (IpcHeader*)buf;
        return h;
    }
    bool decode(const std::string& input) {
        return decode(input.c_str(), input.size());
    }

    bool decode(const char* d, uint32_t len) {
        if (len > (1024)) {
            return false;
        }

        memcpy(buf, d, len);
        return true;
    }

    std::string msg() {
        const char* p = (char*)buf + sizeof(IpcHeader);
        std::string m(p, getHeader()->size);
        return m;
    }

    bool encode(const std::string& input, std::string& output) {
        auto h = getHeader();
        h->size = input.size();
        output.append((const char*)h, sizeof(IpcHeader));
        output.append(input);
        return true;
    }
};

class RouterHandler : public ZeromqRouterHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        IpcPacket recv;
        recv.decode(data);
        std::cout << "router recv:" << recv.msg() << "|" << recv.getHeader()->seqId << std::endl;

        IpcPacket send;
        send.getHeader()->cmd = 2;
        send.getHeader()->seqId = recv.getHeader()->seqId;
        std::string sendMsg;
        send.encode("hello, this is router", sendMsg);

        this->sendData(unique_id, identify, sendMsg);
    }
};

class DealerHandler : public ZeromqDealerHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        const IpcHeader* h = (const IpcHeader*)data.c_str();
        //std::cout << "dealer:" << h->seqId << std::endl;
        co_async::ipc::recv(h->seqId, data.c_str(), data.size());
    }
};

RouterHandler routerHandler;
DealerHandler dealerHandler;
int dealerId = 0;

void ipc_init() {
    routerHandler.listen(1, "tcp://0.0.0.0:1000");
    dealerId = dealerHandler.connect(1, "tcp://127.0.0.1:1000"); 
}

void ipc_send() {
    CoroutineTask::doTask([](void*) {
        // 给router发一个消息
        auto ret = co_async::ipc::send([](uint64_t seqId) {
            // 构造消息包
            IpcPacket send;
            send.getHeader()->seqId = seqId;
            send.getHeader()->cmd = 1;
            
            //std::cout << "dealer seqid:" << seqId << std::endl;
            std::string sendMsg;
            send.encode("hello, this is dealer", sendMsg);
            dealerHandler.sendData(dealerId, sendMsg);
        });

        // 收到回包
        if (co_async::checkOk(ret)) {
            IpcPacket recv;
            recv.decode(ret.second->data, ret.second->dataLen);
            std::cout << "dealer recv:" << recv.msg() << std::endl;
        }
        else {
            std::cout << "dealer error:" << ret.first << std::endl;
        }

    }, 0);
}

void ipc_test() {
    // 初始化
    ipc_init();

    ipc_send();

    while(true) {
        routerHandler.update(0);
        dealerHandler.update(0);
        co_async::loop();
        usleep(1000);
    }
}


#endif