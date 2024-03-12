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
    // ��ʼ�����ӹ��캯��
    Connection();
    // �ر�������������
    ~Connection();
    // ����mysql����
    bool connect(string ip,           // mysqlIP��ַ
                 unsigned short port, // mysql�˿ں�
                 string username,     // mysql�û���
                 string password,     // mysql����
                 string dbname);      // ʹ�õ����ݿ�
    // ��ɾ�Ĳ�������
    bool update(string sql);
    // ��ѯ��������
    MYSQL_RES *query(string sql);
    // ˢ�´����ʼʱ��
    void refreshAliveTime();
    // ��ȡ�Ѿ����ʱ��
    clock_t getAliveTime() const;

private:
    MYSQL *_connection; // mysql����
    clock_t _aliveTime; // �����ʼʱ��
};

#endif