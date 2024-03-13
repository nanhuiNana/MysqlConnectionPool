#include "ConnectionPool.h"
using std::cout;
using std::endl;
using std::move;
using std::this_thread::get_id;
int main() {
    clock_t begin = clock();
    ConnectionPool *pool = ConnectionPool::getInstance();
    thread t[10];
    for (int i = 0; i < 5; i++) {
        thread tt([&]() {
            for (int i = 0; i < 2000; i++) {
                Connection conn = Connection();
                conn.connect("127.0.0.1", 3306, "root", "123456", "mydatabase");
                char sql[1024] = {0};
                sprintf(sql, "insert into user(name,age,sex) values('%s','%d','%s')", "nanhui", rand() % 100, "male");
                conn.update(sql);
                // shared_ptr<Connection> connectionPtr = pool->getConnection();
                // connectionPtr->update(sql);
            }
        });
        t[i] = move(tt);
    }
    for (int i = 0; i < 5; i++) {
        t[i].join();
    }
    clock_t end = clock();
    cout << end - begin << endl;
    return 0;
}