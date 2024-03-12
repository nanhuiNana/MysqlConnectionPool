#include "ConnectionPool.h"

ConnectionPool *ConnectionPool::getInstance() {
    // ��������ģʽ
    static ConnectionPool pool; // �̰߳�ȫ�ľ�̬�ֲ�����
    return &pool;
}

shared_ptr<Connection> ConnectionPool::getConnection() {
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQueue.empty()) {
        // �����ȴ�һ�γ�ʱʱ��
        if (cv_status::timeout == cv.wait_for(lock, milliseconds(_connectionTimeout))) {
            // ��ʱ���ٴμ������Ƿ�Ϊ��
            if (_connectionQueue.empty()) {
                LOG("get connection timeout");
                return nullptr;
            }
        }
    }
    // ʹ������ָ��������ӣ��Զ����ͷ���Դ�����������ӷŻ����Ӷ���
    shared_ptr<Connection> connectionPtr(_connectionQueue.front(),
                                         [&](Connection *connection) {
                                             // cout << "release" << endl;
                                             unique_lock<mutex> lock(_queueMutex);
                                             connection->refreshAliveTime();
                                             _connectionQueue.push(connection);
                                         });
    _connectionQueue.pop();
    cv.notify_all(); // ֪ͨ�����̼߳�����Ӷ����Ƿ�Ϊ��
    return connectionPtr;
}

ConnectionPool::ConnectionPool() {
    // ����������
    if (loadConfigFile() == false) {
        LOG("loadConfigFile error");
        return;
    }

    // ������ʼ�����������ݿ�����
    for (int i = 0; i < _initSize; i++) {
        addConnection();
    }

    flag = true;

    // ���������̣߳�������������
    thread produce(bind(&ConnectionPool::produceThreadCallback, this));
    produce.detach();

    // ������ʱ�߳�,���ڻ��տ�������
    thread timing(bind(&ConnectionPool::aliveTimeThreadCallback, this));
    timing.detach();
}

ConnectionPool ::~ConnectionPool() {
    // cout << "~" << endl;

    while (!_connectionQueue.empty()) {
        // �������ݿ�����
        delete _connectionQueue.front();
        _connectionQueue.pop();
    }
    // cout << "queue:" << _connectionQueue.empty() << endl;
    flag = false;
    cv.notify_all();
    // cout << "~" << endl;
    // sleep_for(seconds(_maxIdleTime * 10));
    // cout << "~" << endl;
}

bool ConnectionPool::loadConfigFile() {
    FILE *fp = fopen("mysql.conf", "r");
    if (fp == nullptr) {
        LOG("mysql.conf is not exist");
        return false;
    }
    string str;
    while (!feof(fp)) {
        char line[1024] = {0};
        fgets(line, 1024, fp);
        str += line;
    }
    json js = json::parse(str);
    _ip = js["ip"];
    _port = js["port"];
    _username = js["username"];
    _password = js["password"];
    _dbname = js["dbname"];
    _initSize = js["initSize"];
    _maxSize = js["maxSize"];
    _maxIdleTime = js["maxIdleTime"];
    _connectionTimeout = js["connectionTimeout"];
    return true;
}

void ConnectionPool::addConnection() {
    Connection *connection = new Connection();
    connection->connect(_ip, _port, _username, _password, _dbname);
    connection->refreshAliveTime();
    _connectionQueue.push(connection);
    _connectionCount++;
}

void ConnectionPool::produceThreadCallback() {
    while (flag) {
        unique_lock<mutex> lock(_queueMutex);
        // cout << "produce queue:" << _connectionQueue.empty() << endl;
        while (!_connectionQueue.empty()) {
            // cout << flag << endl;
            cv.wait(lock); // ���в��գ����������߳�
        }
        if (flag && _connectionCount < _maxSize) {
            addConnection(); // �����������Ӻ���
        }
        cv.notify_all(); // ֪ͨ�����߳�
        // cout << flag << endl;
    }
}

void ConnectionPool::aliveTimeThreadCallback() {
    while (flag) {
        sleep_for(seconds(_maxIdleTime)); // ģ�ⶨʱЧ��
        unique_lock<mutex> lock(_queueMutex);
        while (!_connectionQueue.empty() && _connectionCount > _initSize) {
            Connection *connection = _connectionQueue.front();
            if (connection->getAliveTime() >= (_maxIdleTime * 1000)) {
                _connectionQueue.pop();
                delete connection;
            } else {
                break;
            }
        }
        // cout << flag << endl;
    }
}