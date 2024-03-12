#include "Connection.h"

Connection::Connection() {
    // 初始化mysql连接
    _connection = mysql_init(nullptr);
}

Connection::~Connection() {

    if (_connection != nullptr)
        // 关闭mysql连接
        mysql_close(_connection);
}

bool Connection::connect(string ip, unsigned short port,
                         string username, string password, string dbname) {
    // 数据库连接函数
    MYSQL *p = mysql_real_connect(_connection, ip.c_str(), username.c_str(),
                                  password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(string sql) {
    // 数据库操作函数
    if (mysql_query(_connection, sql.c_str())) {
        LOG("update error: " + sql);
        return false;
    }
    return true;
}

MYSQL_RES *Connection::query(string sql) {
    if (mysql_query(_connection, sql.c_str())) {
        LOG("query error: " + sql);
        return nullptr;
    }
    return mysql_use_result(_connection);
}

void Connection::refreshAliveTime() {
    // 获取程序运行时钟当前的时间
    _aliveTime = clock();
}

clock_t Connection::getAliveTime() const {
    // 返回存活时间=当前时间-起始时间
    return clock() - _aliveTime;
}