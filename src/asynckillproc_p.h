#ifndef ASYNCKILLPROC_P_H
#define ASYNCKILLPROC_P_H

#include <QObject>
#include <QThread>

#include <sys/types.h>

class AsyncKillProcess_Private : public QThread
{
    Q_OBJECT

public:
    typedef QThread super;

private:

    // forbid
    AsyncKillProcess_Private();
    AsyncKillProcess_Private(const AsyncKillProcess_Private &);
    AsyncKillProcess_Private &operator=(const AsyncKillProcess_Private &in);

public:

    explicit AsyncKillProcess_Private(const pid_t &in_pid, char const *const in_kreason, char const *const in_context);
    virtual ~AsyncKillProcess_Private();
    virtual void run();

protected:
    virtual bool event(QEvent *event);

private:

    pid_t m_pid;
    char *m_kreason;
    char *m_context;
    pid_t m_tid;

};

#endif // ASYNCKILLPROC_P_H
