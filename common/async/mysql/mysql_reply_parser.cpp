#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/mysql_reply_parser.h"
#include <mysql/mysql.h>

namespace async {
namespace mysql {

MysqlReplyParser::MysqlReplyParser(const void* res, int affect, int err) {
    this->_res = res;
    this->_errno = err;
    this->_affectedRow = affect;
}

MysqlReplyParser::~MysqlReplyParser() {
    if (this->_res) {
        mysql_free_result((MYSQL_RES*)this->_res);
    }
}

int MysqlReplyParser::GetAffectedRow() {
    if (this->_res && this->_errno == 0) {
        auto rows = (int)mysql_num_rows((MYSQL_RES*)this->_res);
        return rows;
    }   
    else {
        return this->_affectedRow;
    }
}

int MysqlReplyParser::GetField() {
    if (this->_res && this->_errno == 0) {
        auto field = (int)mysql_num_fields((MYSQL_RES*)this->_res);
        return field;
    }   
    else {
        return 0;
    }
}

const void* MysqlReplyParser::Next() {
    if (this->_res && this->_errno == 0) {
        MYSQL_ROW row = mysql_fetch_row((MYSQL_RES*)this->_res);
        return (const void*)row;
    }
    else {
        return 0;
    }
}

int MysqlReplyParser::GetError() {
    return this->_errno;
}

}
}

#endif