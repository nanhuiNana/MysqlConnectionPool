#ifndef __LOG_H__
#define __LOG_H__

// �궨��򵥵���־���
#define LOG(str) \
    cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << " : " << str << endl;

#endif