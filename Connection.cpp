#include "Connection.h"

Connection::Connection() {
    // ��ʼ��mysql����
    _connection = mysql_init(nullptr);
}

Connection::~Connection() {

    if (_connection != nullptr)
        // �ر�mysql����
        mysql_close(_connection);
}

bool Connection::connect(string ip, unsigned short port,
                         string username, string password, string dbname) {
    // ���ݿ����Ӻ���
    MYSQL *p = mysql_real_connect(_connection, ip.c_str(), username.c_str(),
                                  password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(string sql) {
    // ���ݿ��������
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
    // ��ȡ��������ʱ�ӵ�ǰ��ʱ��
    _aliveTime = clock();
}

clock_t Connection::getAliveTime() const {
    // ���ش��ʱ��=��ǰʱ��-��ʼʱ��
    return clock() - _aliveTime;
}