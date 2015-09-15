#ifndef ASYNCKILLPROC_P_H
#define ASYNCKILLPROC_P_H

#include <QObject>
#include <QThread>

#include <sys/types.h>

class AsyncKillProcess_Private : public QThread
{
    Q_OBJECT

public:

    explicit AsyncKillProcess_Private(const pid_t &in_pid, char const *const in_kreason, char const *const in_context);
    virtual ~AsyncKillProcess_Private();
    virtual void run();

private:

    pid_t m_pid;
    char *m_kreason;
    char *m_context;
    pid_t m_tid;

};

#endif // ASYNCKILLPROC_P_H
