#include "singleqprocesssingleshot.h"

#include <QtGlobal>
#include <QCoreApplication>

#include "singleqprocess.h"
#include "util.h"

bool SingleQProcessSingleshot::run(const QString &exe, const QStringList &args, int waitmsecs, QString &error)
{
    if(!this->start(exe, args, error)) {
        error = exe + QLatin1String(" ") + args.join(QLatin1String(" ")) + QLatin1String(": could not be started: ") + error;
        return false;
    }

    if(!this->wait(waitmsecs)) {
        error = exe + QLatin1String(" ") + args.join(QLatin1String(" ")) + QLatin1String(": did not finish");
        return false;
    }

    QCoreApplication::processEvents();

    if(!this->get_success()) {
        error = exe + QLatin1String(" ") + args.join(QLatin1String(" ")) + QLatin1String(": failed:")
                + QLatin1String("\n\n")
                + this->get_errstr()
                + QLatin1String("\n\n")
                + this->get_output()
                ;
        return false;
    }

    return true;
}

bool SingleQProcessSingleshot::start(const QString &exe, const QStringList &args, QString &error)
{
    if(started) {
        PROGRAMMERERROR("start called twice?");
    }

    if(finished) {
        PROGRAMMERERROR("start called after finished?");
    }

    started = true;
    prog = new SingleQProcess(this);
    XCONNECT(prog, SIGNAL(sig_finished(bool, QString, QString, QString, QStringList)), this, SLOT(slot_prog_has_finished(bool, QString, QString, QString, QStringList)), QUEUEDCONN);
    bool ret = prog->start(exe, args, error);
    return ret;
}
void SingleQProcessSingleshot::slot_prog_has_finished(bool success, const QString &errstr, const QString &output, const QString &exe, const QStringList &args)
{
    if(!started) {
        PROGRAMMERERROR("got prog finished signal, but start not called?");
    }

    if(finished) {
        PROGRAMMERERROR("got prog finished signal twice?");
    }

    finished = true;

    m_success = success;
    m_errstr = errstr;
    m_output = output;
    m_exe = exe;
    m_args = args;

    delete prog;
    prog = NULL;
}
bool SingleQProcessSingleshot::wait(int msecs)
{
    if(!started) {
        PROGRAMMERERROR("waiting but not started yet");
    }

    if(finished) {
        return true;
    }

    bool ret = prog->waitForFinished(msecs);
    return ret;
}
