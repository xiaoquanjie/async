#include "serve/serve/base.h"
#include <string.h>
#include <map>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

World::World(uint32_t identify) {
    auto iden = htonl(identify);
    struct in_addr inaddr;
    memcpy(&inaddr, &iden, sizeof(in_addr));
    const char* addr = inet_ntoa(inaddr);
    if (addr) {
        // 按点分割
        std::vector<std::string> ipVec;
        split(addr, ".", ipVec);
        if (ipVec.size() == 4) {
            this->world = atoi(ipVec[1].c_str());
            this->type = atoi(ipVec[2].c_str());
            this->id = atoi(ipVec[3].c_str());
        }
    }
}

bool World::operator <(const World& w) const {
    if (this->world < w.world) {
        return true;
    }
    if (this->world > w.world) {
        return false;
    }
    if (this->type < w.type) {
        return true;
    }
    if (this->type > w.type) {
        return false;
    }
    if (this->id < w.id) {
        return true;
    }
    if (this->id > w.id) {
        return false;
    }
    return false;
}

uint32_t World::identify() const {
    char ipBuf[50] = {0};
    sprintf(ipBuf, "%d.%d.%d.%d", 0, this->world, this->type, this->id);
    auto id = ntohl(inet_addr(ipBuf));
    return id;
}

bool LinkInfo::Item::isTcp() const {
    return (this->protocol == "tcp");
}

bool LinkInfo::Item::isUdp() const {
    return (this->protocol == "udp");
}

std::string LinkInfo::Item::addr() const {
    std::string addr = this->protocol + "://";
    addr += this->ip + ":";
    addr += std::to_string(this->port);
    return addr;
}

uint32_t BackendHeader::size() {
    static uint32_t s = sizeof(BackendHeader);
    return s;
}

void BackendMsg::encode(std::string& output) const {
    output.assign(reinterpret_cast<const char*>(&header), header.size());
	output.append(data);
}

void BackendMsg::decode(const std::string& input) {
    auto p = reinterpret_cast<const BackendHeader*>(input.c_str());
	this->header = *p;
	data.assign(input.c_str() + this->header.size(), input.size() - this->header.size());
}

void HttpRequest::swap(HttpRequest& req) {
    void* tr = this->r;
    this->r = req.r;
    req.r = tr;

    this->url.swap(req.url);
    this->host.swap(req.host);
    this->squery.swap(req.squery);
    this->body.swap(req.body);
    this->query.swap(req.query);
}

void HttpRespond::swap(HttpRespond& rsp) {
    this->body.swap(rsp.body);
    this->header.swap(rsp.header);
}

void split(const std::string source, const std::string &separator, std::vector<std::string> &array) {
    array.clear();
    std::string::size_type start = 0;
    while (true) {
        std::string::size_type pos = source.find(separator, start);
        if (pos == std::string::npos) {
            std::string sub = source.substr(start, source.size());
            array.push_back(sub);
            break;
        }

        std::string sub = source.substr(start, pos - start);
        start = pos + separator.size();
        array.push_back(sub);
    }
}