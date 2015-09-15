#include "singleqprocess.h"

#include <QProcess>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QLatin1String>
#include <QCoreApplication>

#include "encoding.h"
#include "safe_signals.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "SQP"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static const int timeout_waitforstarted = 3000;

SingleQProcess::SingleQProcess(QObject *parent, const QString &in_oname) :
    QObject(parent), proc(NULL)
{
    setObjectName(in_oname);
}

SingleQProcess::~SingleQProcess()
{
    close();
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
    proc = new QProcess();
    proc->setObjectName(objectName() + QLatin1String("_QP"));
    qRegisterMetaType<QProcess::ExitStatus>();
    XCONNECT(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_child_has_finished(int, QProcess::ExitStatus)), QUEUEDCONN);
    XCONNECT(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(slot_accumulate_all_out()), QUEUEDCONN);

    proc->setProcessChannelMode(QProcess::MergedChannels);
    // proc->setProcessChannelMode(QProcess::ForwardedChannels);
    MYDBG("Executing %s %s", qPrintable(exe), qPrintable(args.join(QLatin1String(" "))));
    proc->start(exe, args);

    bool isstarted = proc->waitForStarted(timeout_waitforstarted);
    QCoreApplication::processEvents();

    if(!isstarted) {
        slot_accumulate_all_out();

        if(accout.isEmpty()) {
            error = QLatin1String("viewer could not start, no output?");
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
    if(exitCode == 0 && status == QProcess::NormalExit) {
        emit sig_finished(true, QString(), output_till_now(), curr_exe, curr_args);
        return;
    }

    QString error;

    if(exitCode != 0) {
        error.append(QLatin1String("exitCode: "));
        QString sExitCode;
        sExitCode.setNum(exitCode);
        error.append(sExitCode);
    }

    if(status != QProcess::NormalExit) {
        if(!error.isEmpty()) {
            error.append(QLatin1String("; "));
        }

        error.append(QLatin1String("process crashed"));
    }

    if(error.isEmpty()) {
        error.append(QLatin1String("unknown problem"));
    }

    emit sig_finished(false, error, output_till_now(), curr_exe, curr_args);
}

bool SingleQProcess::waitForFinished(int msecs)
{
    bool ret = proc->waitForFinished(msecs);
    QCoreApplication::processEvents();
    return ret;
}


