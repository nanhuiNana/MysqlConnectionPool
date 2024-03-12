#ifndef __CONNECTIONPOOL_H__
#define __CONNECTIONPOOL_H__
#include "Connection.h"
#include "json.hpp"
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
using json = nlohmann::json;
using std::bind;
using std::condition_variable;
using std::cv_status;
using std::FILE;
using std::mutex;
using std::queue;
using std::shared_ptr;
using std::thread;
using std::unique_lock;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::this_thread::sleep_for;
class ConnectionPool {
public:
    // 获取连接池对象实例接口
    static ConnectionPool *getInstance();
    // 从连接池中获取数据库连接接口，使用智能指针进行管理
    shared_ptr<Connection> getConnection();

private:
    // 构造函数私有化
    ConnectionPool();
    // 析构函数销毁数据库连接池
    ~ConnectionPool();
    // 删除拷贝构造和赋值构造
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;
    // 从配置文件中加载配置项函数
    bool loadConfigFile();
    // 生产连接封装函数
    void addConnection();
    // 生产线程回调函数
    void produceThreadCallback();
    // 空闲连接检测线程回调函数
    void aliveTimeThreadCallback();

    string _ip;             // mysql的ip地址
    unsigned short _port;   // mysql的端口号
    string _username;       // mysql登录用户名
    string _password;       // mysql登录密码
    string _dbname;         // 连接的数据库名称
    int _initSize;          // 连接池的初始连接量
    int _maxSize;           // 连接池的最大连接量
    int _maxIdleTime;       // 连接的最大空闲时间
    int _connectionTimeout; // 获取连接的超时时间

    bool flag;

    queue<Connection *> _connectionQueue; // 存储数据库连接的队列
    mutex _queueMutex;                    // 维护连接队列线程安全的互斥锁
    int _connectionCount;                 // 记录连接池的连接总量
    condition_variable cv;                // 条件变量，用于生产线程和工作（消费）线程的通信同步操作
};

#endif