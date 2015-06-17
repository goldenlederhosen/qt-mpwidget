#include "asynckillproc.h"
#include "asynckillproc_p.h"
#include "safe_signals.h"

void async_kill_process(const pid_t &in_pid, char const *const in_kreason, char const *const in_context)
{
    AsyncKillProcess_Private *akp = new AsyncKillProcess_Private(in_pid, in_kreason, in_context);
    XCONNECT(akp, SIGNAL(finished()), akp, SLOT(deleteLater()), QUEUEDCONN);
    akp->start();
}
