#include "ConnectionPool.h"

ConnectionPool *ConnectionPool::getInstance() {
    // 懒汉单例模式
    static ConnectionPool pool; // 线程安全的静态局部变量
    return &pool;
}

shared_ptr<Connection> ConnectionPool::getConnection() {
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQueue.empty()) {
        // 阻塞等待一段超时时间
        if (cv_status::timeout == cv.wait_for(lock, milliseconds(_connectionTimeout))) {
            // 超时后再次检测队列是否为空
            if (_connectionQueue.empty()) {
                LOG("get connection timeout");
                return nullptr;
            }
        }
    }
    // 使用智能指针管理连接，自定义释放资源函数，将连接放回连接队列
    shared_ptr<Connection> connectionPtr(_connectionQueue.front(),
                                         [&](Connection *connection) {
                                             // cout << "release" << endl;
                                             unique_lock<mutex> lock(_queueMutex);
                                             connection->refreshAliveTime();
                                             _connectionQueue.push(connection);
                                         });
    _connectionQueue.pop();
    cv.notify_all(); // 通知生产线程检查连接队列是否为空
    return connectionPtr;
}

ConnectionPool::ConnectionPool() {
    // 加载配置项
    if (loadConfigFile() == false) {
        LOG("loadConfigFile error");
        return;
    }

    // 创建初始连接量的数据库连接
    for (int i = 0; i < _initSize; i++) {
        addConnection();
    }

    flag = true;

    // 创建生产线程，用于生产连接
    thread produce(bind(&ConnectionPool::produceThreadCallback, this));
    produce.detach();

    // 创建定时线程,用于回收空闲连接
    thread timing(bind(&ConnectionPool::aliveTimeThreadCallback, this));
    timing.detach();
}

ConnectionPool ::~ConnectionPool() {
    // cout << "~" << endl;

    while (!_connectionQueue.empty()) {
        // 销毁数据库连接
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
            cv.wait(lock); // 队列不空，阻塞生产线程
        }
        if (flag && _connectionCount < _maxSize) {
            addConnection(); // 调用生产连接函数
        }
        cv.notify_all(); // 通知工作线程
        // cout << flag << endl;
    }
}

void ConnectionPool::aliveTimeThreadCallback() {
    while (flag) {
        sleep_for(seconds(_maxIdleTime)); // 模拟定时效果
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