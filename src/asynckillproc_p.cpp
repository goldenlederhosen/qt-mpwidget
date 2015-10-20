#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>

#include "asynckillproc_p.h"
#include "util.h"
#include "event_desc.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "AKP"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, "%s on PID=%d TID=%d " msg, m_context, m_pid, m_tid, ##__VA_ARGS__)

AsyncKillProcess_Private::AsyncKillProcess_Private(const pid_t &in_pid, char const *const in_kreason, char const *const in_context):
    super(NULL),
    m_pid(in_pid),
    m_kreason(::strdup(in_kreason)),
    m_context(::strdup(in_context)),
    m_tid(-1)
{
    setObjectName(QStringLiteral("AsyncKillProcess_Private_") + QString::number(m_pid));

    MYDBG("CONSTRUCT");

    if(m_kreason == NULL) {
        throw std::bad_alloc();
    }

    if(m_context == NULL) {
        throw std::bad_alloc();
    }

}
AsyncKillProcess_Private::~AsyncKillProcess_Private()
{
    MYDBG("DESTRUCT");
    ::free(m_kreason);
    ::free(m_context);
}

bool AsyncKillProcess_Private::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}


void AsyncKillProcess_Private::run()
{
    m_tid = syscall(SYS_gettid);
    MYDBG("run()");

    MYDBG("killing: %s", m_kreason);
    const int SIG = 9;
    const int killret = ::kill(m_pid, SIG);

    if(killret != 0) {
        if(errno != ESRCH) {
            qWarning("could not kill child %s: %s", m_kreason, strerror(errno));
        }

        return;
    }

    // successfully killed
    MYDBG("now waiting for it to die");
    int status = 0;
    pid_t p = ::waitpid(m_pid, &status, 0);

    if(p != m_pid) {
        MYDBG("waitpid() returned %d: %s", p, strerror(errno));
        return;
    }

    if(WIFEXITED(status)) {
        int ex = WEXITSTATUS(status);
        MYDBG("exited with value %d", ex);
        (void) ex;
        return;
    }

    if(WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);

        if(sig != SIG) {
            MYDBG("died from signal %d %s (I really expected %d %s)", sig, strsignal(sig), SIG, strsignal(SIG));
        }
        else {
            MYDBG("died from signal %d %s", sig, strsignal(sig));
        }

        return;
    }

    MYDBG("status is %d, do not know what that means", status);


    return;
}

