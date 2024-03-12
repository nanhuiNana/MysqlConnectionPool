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
    // ��ȡ���ӳض���ʵ���ӿ�
    static ConnectionPool *getInstance();
    // �����ӳ��л�ȡ���ݿ����ӽӿڣ�ʹ������ָ����й���
    shared_ptr<Connection> getConnection();

private:
    // ���캯��˽�л�
    ConnectionPool();
    // ���������������ݿ����ӳ�
    ~ConnectionPool();
    // ɾ����������͸�ֵ����
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;
    // �������ļ��м����������
    bool loadConfigFile();
    // �������ӷ�װ����
    void addConnection();
    // �����̻߳ص�����
    void produceThreadCallback();
    // �������Ӽ���̻߳ص�����
    void aliveTimeThreadCallback();

    string _ip;             // mysql��ip��ַ
    unsigned short _port;   // mysql�Ķ˿ں�
    string _username;       // mysql��¼�û���
    string _password;       // mysql��¼����
    string _dbname;         // ���ӵ����ݿ�����
    int _initSize;          // ���ӳصĳ�ʼ������
    int _maxSize;           // ���ӳص����������
    int _maxIdleTime;       // ���ӵ�������ʱ��
    int _connectionTimeout; // ��ȡ���ӵĳ�ʱʱ��

    bool flag;

    queue<Connection *> _connectionQueue; // �洢���ݿ����ӵĶ���
    mutex _queueMutex;                    // ά�����Ӷ����̰߳�ȫ�Ļ�����
    int _connectionCount;                 // ��¼���ӳص���������
    condition_variable cv;                // �������������������̺߳͹��������ѣ��̵߳�ͨ��ͬ������
};

#endif