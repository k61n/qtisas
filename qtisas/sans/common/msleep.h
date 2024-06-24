
#ifndef MSLEEP_H
#define MSLEEP_H

#include <QThread>

class SleepThread : public QThread {
public:
static inline void msleep(unsigned long msecs) {
QThread::msleep(msecs);
}
};

#endif
