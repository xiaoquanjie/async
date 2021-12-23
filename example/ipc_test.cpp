#ifdef USE_IPC

#include "./common.h"

class RouterHandler : public ZeromqRouterHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        std::cout << "router:" << unique_id << "|" << identify <<"|" << data << std::endl;
        std::string rsp_data = ("router resoond " + data);
        this->sendData(unique_id, identify, rsp_data);
    }
};

class DealerHandler : public ZeromqDealerHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        std::cout << "dealer:" << unique_id << "|" << identify <<"|" << data << std::endl;
    }
};

void ipc_test() {
    RouterHandler routerHandler;
    DealerHandler dealerHandler;
    routerHandler.listen(1, "tcp://0.0.0.0:1000");
    routerHandler.listen(2, "tcp://0.0.0.0:1001");
    int id1 = dealerHandler.connect(1, "tcp://127.0.0.1:1000"); 
    int id2 = dealerHandler.connect(2, "tcp://127.0.0.1:1001"); 
    dealerHandler.sendData(id2, "hello world");
    dealerHandler.sendData(id1, "hello world2");
    while (true) {
        routerHandler.update(0);
        dealerHandler.update(0);
        usleep(1);
    }
}

#endif