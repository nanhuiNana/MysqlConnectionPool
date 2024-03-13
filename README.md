# MysqlConnectionPool
一个基于C++11实现的MySQL数据库连接池
## 数据库配置
* 需要在mysql.conf文件中dbname字段对应值修改为本机数据库名称，确保该数据库中存在user用户表，user表存在name,age,sex三个字段，才能进行正常的测试操作，这些配置可自定义
## 环境配置
* 需要安装MySQL服务器，安装libmysqlclient-dev开发库
## 编译命令
```shell
g++ Connection.cpp ConnectionPool.cpp main.cpp -o main -lmysqlclient -pthread
```
## 封装数据库
### 封装数据库连接
```cpp
bool Connection::connect(string ip, unsigned short port, string username, 
                        string password, string dbname) {
    // 数据库连接函数
    _connection = mysql_real_connect(_connection, ip.c_str(), 
                                    username.c_str(),
                                    password.c_str(), 
                                    dbname.c_str(), port, 
                                    nullptr, 0);
    return _connection != nullptr;
}
```
### 封装数据库操作（增删改查）
```cpp
//增删改
bool Connection::update(string sql) {
// 数据库操作函数
    if (mysql_query(_connection, sql.c_str())) {
        LOG("update error: " + sql);
        return false;
    }
    return true;
}

//查询
MYSQL_RES *Connection::query(string sql) {
    if (mysql_query(_connection, sql.c_str())) {
        LOG("query error: " + sql);
        return nullptr;
    }
    return mysql_use_result(_connection);
}
```
### 设置数据库存活时间操作函数
- 用于判断创建出来的数据库连接是否空闲着一直未被使用
```cpp
void Connection::refreshAliveTime() {
    // 获取程序运行时钟当前的时间
    _aliveTime = clock();
}

clock_t Connection::getAliveTime() const {
    // 返回存活时间=当前时间-起始时间
    return clock() - _aliveTime;
}
```
## 封装数据库连接池
### 单例模式
- 因为连接池只需要一个实例，使用单例模式进行设计
```cpp
class ConnectionPool {
public:
    // 获取连接池对象实例接口
    static ConnectionPool *getInstance();

private:
    // 构造函数私有化
    ConnectionPool();
    //删除拷贝构造和赋值构造
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;
}
```
### 连接队列
- 空闲连接全部维护在一个线程安全的连接队列中，使用线程互斥锁保证队列的线程安全
```cpp
private:
    queue<Connection *> _connectionQueue; // 存储数据库连接的队列
    mutex _queueMutex;                    // 维护连接队列线程安全的互斥锁
```
### 获取连接接口
- 从连接池中可以获取和MySQL的连接，如果连接队列为空，等待`connectionTimeout`时间还获取不到空闲的连接，那么获取连接失败，用户获取的连接用`shared_ptr`智能指针来管理，用lambda表达式定制连接释放的功能
```cpp
public:
    // 从连接池中获取数据库连接接口，使用智能指针进行管理
    shared_ptr<Connection> getConnection();
    int _connectionTimeout; // 获取连接的超时时间
    
shared_ptr<Connection> ConnectionPool::getConnection() {
    unique_lock<mutex> lock(_queueMutex); // 上锁，出作用域后会自动解锁
    while (_connectionQueue.empty()) { // 判断连接队列是否为空
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
                                             unique_lock<mutex> lock(_queueMutex); // 上锁
                                             connection->refreshAliveTime(); // 刷新空闲起始时间
                                             _connectionQueue.push(connection); // 归还连接到队列中
                                         });
    _connectionQueue.pop(); // 弹出连接
    cv.notify_all(); // 通知生产线程检查连接队列是否为空
    return connectionPtr; // 返回智能指针
}
```
### 封装生产连接
- 封装生产连接的相关操作，使构造函数和生产连接的线程都可以复用
```cpp
void ConnectionPool::addConnection() {
    Connection *connection = new Connection(); // 创建新连接
    connection->connect(_ip, _port, _username, _password, _dbname); // 初始化连接
    connection->refreshAliveTime(); // 刷新存活起始时间
    _connectionQueue.push(connection); // 放入连接队列
    _connectionCount++; // 更新连接总量
}
```
### 生产连接线程
- 如果连接队列为空，还需要再获取连接，此时需要动态创建连接，设置一个专门生产连接的线程，生产上限数量是maxSize
- 连接的生产和连接的消费采用生产者-消费者线程模型来设计，使用了线程间的同步通信机制条件变量
```cpp
private:
    int _maxSize;           // 连接池的最大连接量
    int _connectionCount;   // 记录连接池的连接总量
    condition_variable cv;  // 条件变量，用于生产线程和工作（消费）线程的通信同步操作
    // 生产线程回调函数
    void produceThreadCallback();

void ConnectionPool::produceThreadCallback() {
    while (flag) {
        unique_lock<mutex> lock(_queueMutex); // 上锁，出作用域后会自动解锁
        while (!_connectionQueue.empty()) {
            cv.wait(lock); // 队列不空，阻塞生产线程
        }
        if (flag && _connectionCount < _maxSize) { // 连接数量没有达到上限并且flag线程开关为true
            addConnection(); // 生产连接
        }
        cv.notify_all(); // 通知工作线程
    }
}
```
### 定时检测空闲连接线程
- 队列中连接的空闲时间超过maxIdleTime就要被释放掉，只保留初始的initSize个连接就可以了
```cpp
private:
    int _maxIdleTime;       // 连接的最大空闲时间
    int _initSize;          // 连接池的初始连接量
    // 空闲连接检测线程回调函数
    void aliveTimeThreadCallback();
    
void ConnectionPool::aliveTimeThreadCallback() {
    while (flag) {
        sleep_for(seconds(_maxIdleTime)); // 模拟定时效果
        unique_lock<mutex> lock(_queueMutex); // 上锁，出作用域后会自动解锁
        while (!_connectionQueue.empty() && _connectionCount > _initSize) { // 如果连接队列不空并且连接总量超过了初始连接量
            Connection *connection = _connectionQueue.front(); // 取出队头连接
            if (connection->getAliveTime() >= (_maxIdleTime * 1000)) { // 查看队头连接是否一直空闲着超过空闲时间
                _connectionQueue.pop(); // 弹出队头连接
                delete connection; // 销毁连接 
            } else {
                break; // 如果检测到没有超过空闲时间的连接直接跳出，说明后面的所有连接都不会超过空闲时间
            }
        }
    }
}
```
### 数据库相关配置
- 数据库相关配置使用json数据格式进行保存，使用文件操作进行存储和读取
```cpp
private:
    string _ip;             // mysql的ip地址
    unsigned short _port;   // mysql的端口号
    string _username;       // mysql登录用户名
    string _password;       // mysql登录密码
    string _dbname;         // 连接的数据库名称
    // 从配置文件中加载配置项函数
    bool loadConfigFile();

bool ConnectionPool::loadConfigFile() {
    FILE *fp = fopen("mysql.conf", "r");
    if (fp == nullptr) {
        LOG("mysql.conf is not exist");
        return false;
    }
    string str;
    //文件操作
    while (!feof(fp)) {
        char line[1024] = {0};
        fgets(line, 1024, fp);
        str += line;
    }
    json js = json::parse(str); // 将字符串反序列化为json对象
    // 读取数据库相关配置
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
```
```json
//mysql.conf配置文件
{
    "ip" : "127.0.0.1",
    "port" : 3306,
    "username" : "root",
    "password" : "123456",
    "dbname" : "mydatabase",
    "initSize" : 5,
    "maxSize" : 1024,
    "maxIdleTime" : 60,
    "connectionTimeout" : 100
}
```
### 构造函数初始化
- 构造函数进行相关初始化操作
```cpp
ConnectionPool::ConnectionPool() {
    // 加载配置项
    if (loadConfigFile() == false) {
        LOG("loadConfigFile error");
        return;
    }

    // 创建初始连接量的数据库连接
    for (int i = 0; i < _initSize; i++) {
        addConnection(); // 生产连接
    }

    flag = true; // 线程开关，用于关闭生产线程和定时线程

    // 创建生产线程，用于生产连接
    thread produce(bind(&ConnectionPool::produceThreadCallback, this));
    produce.detach();

    // 创建定时线程,用于回收空闲连接
    thread timing(bind(&ConnectionPool::aliveTimeThreadCallback, this));
    timing.detach();
}
```
### 析构函数销毁
- 析构函数进行相关释放和销毁操作
```cpp
ConnectionPool ::~ConnectionPool() {
    while (!_connectionQueue.empty()) { // 判断连接队列是否为空
        // 销毁数据库连接
        delete _connectionQueue.front();
        _connectionQueue.pop(); // 连接出队
    }
    flag = false; // 线程开关关闭，让定时线程和生产线程都可以关闭自身
    cv.notify_all(); //通知生产线程，让其关闭自身
}
```