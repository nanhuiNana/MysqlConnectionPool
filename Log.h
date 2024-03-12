#ifndef __LOG_H__
#define __LOG_H__

// 宏定义简单的日志输出
#define LOG(str) \
    cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << " : " << str << endl;

#endif