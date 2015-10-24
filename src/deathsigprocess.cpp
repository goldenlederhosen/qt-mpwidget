#include "deathsigprocess.h"

#include "event_desc.h"
#include "safe_signals.h"
#include "qprocess_meta.h"
#include "asynckillproc.h"

#include <sys/prctl.h>
#include <string.h>
#include <stdio.h>

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "DSP"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

DeathSigProcess::DeathSigProcess(char const *const oName_latin1, QObject *parent)
    : super()
{
    QString OnO((QLatin1String(oName_latin1)));
    init(OnO, parent);
}
DeathSigProcess::DeathSigProcess(const QString &oName, QObject *parent)
    : super()
{
    init(oName, parent);
}

DeathSigProcess::~DeathSigProcess()
{
    if(this->state() == QProcess::Running) {
        const qint64 pid = this->processId();
        MYDBG("%s destructor: killing PID %ld", qPrintable(objectName()), (long)pid);
        const QString oN = objectName();
        this->close();
        async_kill_process(pid, "destructor", qPrintable(oN));
    }
}

void DeathSigProcess::init(const QString &oName, QObject *parent)
{
    setObjectName(oName);
    setParent(parent);
    qRegisterMetaType<QProcess::ProcessError>();
    XCONNECT(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slot_dbg_error(QProcess::ProcessError)));
    XCONNECT(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_dbg_finished(int, QProcess::ExitStatus)));
    XCONNECT(this, SIGNAL(readyReadStandardError()), this, SLOT(slot_dbg_readyReadStandardError()));
    XCONNECT(this, SIGNAL(readyReadStandardOutput()), this, SLOT(slot_dbg_readyReadStandardOutput()));
    XCONNECT(this, SIGNAL(started()), this, SLOT(slot_dbg_started()));
    XCONNECT(this, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(slot_dbg_stateChanged(QProcess::ProcessState)));
    XCONNECT(this, SIGNAL(aboutToClose()), this, SLOT(slot_dbg_aboutToClose()));
    XCONNECT(this, SIGNAL(bytesWritten(qint64)), this, SLOT(slot_dbg_bytesWritten(qint64)));
    XCONNECT(this, SIGNAL(readChannelFinished()), this, SLOT(slot_dbg_readChannelFinished()));
    XCONNECT(this, SIGNAL(readyRead()), this, SLOT(slot_dbg_readyRead()));
}

void DeathSigProcess::slot_dbg_error(QProcess::ProcessError error)
{
    char const *const s = ProcessError_2_latin1str(error);
    MYDBG("%s %ld error(%s)", qPrintable(objectName()), (long)(this->processId()), s);
}
void DeathSigProcess::slot_dbg_finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    MYDBG("%s %ld finished(%d, %s)", qPrintable(objectName()), (long)(this->processId()), exitCode, exitStatus == QProcess::NormalExit ? "normalexit" : "crashexit");
}
void DeathSigProcess::slot_dbg_readyReadStandardError()
{
    MYDBG("%s %ld readyReadStandardError()", qPrintable(objectName()), (long)(this->processId()));
}
void DeathSigProcess::slot_dbg_readyReadStandardOutput()
{
    // MYDBG("%s %ld readyReadStandardOutput()", qPrintable(objectName()));
}
void DeathSigProcess::slot_dbg_started()
{
    MYDBG("%s %ld started()", qPrintable(objectName()), (long)(this->processId()));
}
void DeathSigProcess::slot_dbg_stateChanged(QProcess::ProcessState newState)
{
    MYDBG("%s %ld stateChanged(%s)", qPrintable(objectName()), (long)(this->processId()), newState == QProcess::NotRunning ? "NotRunning" : (newState == QProcess::Starting ? "Starting" : "Running"));
}

void DeathSigProcess::slot_dbg_aboutToClose()
{
    MYDBG("%s %ld aboutToClose()", qPrintable(objectName()), (long)(this->processId()));
}
void DeathSigProcess::slot_dbg_bytesWritten(qint64 bytes)
{
    MYDBG("%s %ld bytesWritten(%ld)", qPrintable(objectName()), (long)(this->processId()), (long)bytes);
}
void DeathSigProcess::slot_dbg_readChannelFinished()
{
    MYDBG("%s %ld readChannelFinished()", qPrintable(objectName()), (long)(this->processId()));
}
void DeathSigProcess::slot_dbg_readyRead()
{
    // MYDBG("%s %ld readyRead()", qPrintable(objectName()));
}
bool DeathSigProcess::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}
void DeathSigProcess::setupChildProcess()
{
    // int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
    int ret = prctl(PR_SET_PDEATHSIG, 9);

    if(ret == 0) {
        fprintf(stderr, THIS_SOURCE_FILE_LOG_CATEGORY " %s %ld prctl(PR_SET_PDEATHSIG, 9) = success\n", qPrintable(objectName()), (long)(this->processId()));
    }
    else {
        fprintf(stderr, THIS_SOURCE_FILE_LOG_CATEGORY " %s %ld prctl(PR_SET_PDEATHSIG, 9) = %d (%s)\n", qPrintable(objectName()), (long)(this->processId()), ret, strerror(errno));
    }
}
