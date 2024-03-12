#ifndef __CONNECTION_H__
#define __CONNECTION_H__
#include "Log.h"
#include <ctime>
#include <iostream>
#include <mysql/mysql.h>
#include <string>
using std::cout;
using std::endl;
using std::string;

class Connection {
public:
    // 初始化连接构造函数
    Connection();
    // 关闭连接析构函数
    ~Connection();
    // 连接mysql函数
    bool connect(string ip,           // mysqlIP地址
                 unsigned short port, // mysql端口号
                 string username,     // mysql用户名
                 string password,     // mysql密码
                 string dbname);      // 使用的数据库
    // 增删改操作函数
    bool update(string sql);
    // 查询操作函数
    MYSQL_RES *query(string sql);
    // 刷新存活起始时间
    void refreshAliveTime();
    // 获取已经存活时间
    clock_t getAliveTime() const;

private:
    MYSQL *_connection; // mysql连接
    clock_t _aliveTime; // 存活起始时间
};

#endif