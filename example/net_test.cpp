#ifdef USE_NET

#include "./common.h"

void net_test() {
    std::cout << "action\n";
    net::UdpListener l(0);
    if (!l.listen("0.0.0.0:3001")) {
        std::cout << "failed to listen\n";
        return;
    }
    l.setDataCb([](net::UdpListener* l, uint32_t fd, const char* data, uint32_t len) {
        std::cout << "[server]:" << std::string(data, len) << std::endl;
        std::string d = "i am server";
        l->send(fd, d.c_str(), d.length());
        sleep(1);
    });

    net::UdpClient c(0);
    if (!c.connect("127.0.0.1:3001")) {
        std::cout << "failed to connect\n";
        return;
    }
    c.setDataCb([](net::UdpClient* c, const char* data, uint32_t len) {
        std::cout << "[client]:" << std::string(data, len) << std::endl;
        std::string d = "i am client";
        c->send(d.c_str(), d.length());
        sleep(1);
    });
    c.send("action", 6);

    try
    {
        while (true)
        {
            l.update(0);
            c.update(0);
            usleep(1000 * 100);
        }
    }
    catch (...)
    {
        std::cout << "exce\n";
    }

    std::cout << "over\n";
}

#endif