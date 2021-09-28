#define USE_ASYNC_MYSQL
#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/async_mysql_client.h"
#include <vector>
#include "mysql.h"
#include <mariadb/mysqld_error.h>
#include <mariadb/errmsg.h>
#include <assert.h>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mysql {

// mysql地址结构
struct mysql_addr {
    mysql_addr (const std::string& uri) {
        Parse(uri);
    }

    mysql_addr () {}

    void Parse(const std::string& uri) {
        this->uri = uri;
        std::vector<std::string> values;
        async::split(uri, "|", values);
        if (values.size() == 5) {
            this->host = values[0];
            this->port = atoi(values[1].c_str());
            this->db = values[2];
            this->user = values[3];
            this->pwd = values[4];
        }
        else {
            printf("uri error:%s\n", uri.c_str());
            assert(false);
        }
    }

    std::string uri;
    std::string host;
    unsigned int port = 0;
    std::string db;
    std::string user;
    std::string pwd;
};

// mysql连接状态
enum {
    enum_connecting_state = 0,
    enum_connected_state,
    enum_disconnected_state,
};

// mysql连接核心
struct mysql_core {
    MYSQL mysql;
    mysql_addr addr;
    int state = enum_connecting_state;

    mysql_core() {}

    ~mysql_core() {
        mysql_close(&mysql);
    }
};

}    
}

#endif