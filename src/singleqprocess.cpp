#include "singleqprocess.h"

#include <QProcess>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QCoreApplication>

#include "encoding.h"
#include "safe_signals.h"
#include "event_desc.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "SQP"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static_var const int timeout_waitforstarted = 3000;

SingleQProcess::SingleQProcess(QObject *parent, const QString &in_oname) :
    super(), proc(NULL)
{
    setObjectName(in_oname);
    setParent(parent);
}

SingleQProcess::~SingleQProcess()
{
    close();
}

bool SingleQProcess::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}

void SingleQProcess::close()
{
    if(proc != NULL) {
        proc->close();
        delete proc;
        proc = NULL;
    }

    accout.clear();
}

void SingleQProcess::slot_accumulate_all_out()
{
    MYDBG("slot_accumulate_all_out");
    next = proc->readAllStandardOutput();
    printf("%s", next.constData());
    accout.append(next);
}

QString SingleQProcess::output_till_now() const
{
    return warn_xbin_2_local_qstring(accout);
}

bool SingleQProcess::start(const QString &exe, const QStringList &args, QString &error)
{
    close();
    curr_exe = exe;
    curr_args = args;
    proc = new DeathSigProcess(objectName() + QLatin1String("_QP"), NULL);
    qRegisterMetaType<QProcess::ExitStatus>();
    XCONNECT(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_child_has_finished(int, QProcess::ExitStatus)), QUEUEDCONN);
    XCONNECT(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(slot_accumulate_all_out()), QUEUEDCONN);

    proc->setProcessChannelMode(QProcess::MergedChannels);
    // proc->setProcessChannelMode(QProcess::ForwardedChannels);
    MYDBG("Executing %s %s", qPrintable(exe), qPrintable(args.join(QStringLiteral(" "))));
    proc->start(exe, args);

    bool isstarted = proc->waitForStarted(timeout_waitforstarted);
    QCoreApplication::processEvents();

    if(!isstarted) {
        slot_accumulate_all_out();

        if(accout.isEmpty()) {
            error = QStringLiteral("viewer could not start, no output?");
        }
        else {
            error = output_till_now();
        }

        return false;
    }

    return true;

}

void SingleQProcess::slot_child_has_finished(int exitCode, QProcess::ExitStatus status)
{
    MYDBG("slot_child_has_finished(%d, %s)", exitCode, status == QProcess::NormalExit ? "normal" : "crash");

    if(exitCode == 0 && status == QProcess::NormalExit) {
        MYDBG("emit sig_finished(true)");
        emit sig_finished(true, QString(), output_till_now(), curr_exe, curr_args);
        return;
    }

    QString error;

    if(exitCode != 0) {
        error.append(QStringLiteral("exitCode: "));
        QString sExitCode;
        sExitCode.setNum(exitCode);
        error.append(sExitCode);
    }

    if(status != QProcess::NormalExit) {
        if(!error.isEmpty()) {
            error.append(QStringLiteral("; "));
        }

        error.append(QStringLiteral("process crashed"));
    }

    if(error.isEmpty()) {
        error.append(QStringLiteral("unknown problem"));
    }

    MYDBG("emit sig_finished(false, %s)", qPrintable(error));
    emit sig_finished(false, error, output_till_now(), curr_exe, curr_args);
}

bool SingleQProcess::waitForFinished(int msecs)
{
    bool ret = proc->waitForFinished(msecs);
    QCoreApplication::processEvents();
    return ret;
}


