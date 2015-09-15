#include "cropdetector.h"
#include "safe_signals.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "CD"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

CropDetector::CropDetector(QObject *parent, QString in_mfn) :
    QObject(parent),
    mfn(in_mfn),
    proc(this)
{

    QString fshort = mfn.right(20).simplified().replace(QLatin1Char(' '), QLatin1Char('_'));
    setObjectName(QLatin1String("CropDetector_") + fshort);
    proc.setObjectName(QLatin1String("CropDetector_QP_") + fshort);

    // add dir of own exe to PATH
    QString program = QLatin1String("videofile.info");
    QStringList args;
    args << QLatin1String("-crop");
    args << mfn;
    proc.start(program, args);
    start = QDateTime::currentDateTimeUtc();

    XCONNECT(&proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slot_pError(QProcess::ProcessError)), QUEUEDCONN);
    XCONNECT(&proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_pfinished(int, QProcess::ExitStatus)), QUEUEDCONN);
}

static char const *ProcessError_2_latin1str(QProcess::ProcessError e)
{
    switch(e) {
        case QProcess::FailedToStart:
            return "The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.";

        case QProcess::Crashed:
            return "The process crashed some time after starting successfully.";

        case QProcess::Timedout:
            return "The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.";

        case QProcess::WriteError:
            return "An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.";

        case QProcess::ReadError:
            return "An error occurred when attempting to read from the process. For example, the process may not be running.";

        case QProcess::UnknownError:
            return "An unknown error occurred. This is the default return value of error().";

        default:
            return "unknown error";
    }
}
void CropDetector::slot_pError(QProcess::ProcessError e)
{
    char const *const be = ProcessError_2_latin1str(e);
    const QString se = QLatin1String(be);
    const QByteArray berr = proc.readAllStandardError();
    const QString serr = QString::fromLocal8Bit(berr.constData()).simplified();
    emit sig_detected(false, se + QChar(QLatin1Char('\n')) + serr, mfn);
}

void CropDetector::slot_pfinished(int ecode, QProcess::ExitStatus estatus)
{
    const QByteArray berr = proc.readAllStandardError();
    const QString serr = QString::fromLocal8Bit(berr.constData()).simplified();

    const QByteArray bout = proc.readAllStandardOutput();
    const QString sout = QString::fromLocal8Bit(bout.constData()).simplified();

    QDateTime end = QDateTime::currentDateTimeUtc();
    qint64 duration = start.msecsTo(end);
    MYDBG("videofile.info -crop \"%s\" finished, duration=%ldmsec out=\"%s\" err=\"%s\"", qPrintable(mfn), (long int) duration, qPrintable(sout), qPrintable(serr));

    static QRegExp rxcs(QLatin1String("^(\\d+):(\\d+):(\\d+):(\\d+)$"));

    if(!rxcs.isValid()) {
        PROGRAMMERERROR("WTF");
    }

    QStringList errs;

    if(ecode != 0) {
        QString errmsg = QString(QLatin1String("videofile.info failed with \"%1\"")).arg(ecode);
        errs.append(errmsg);
    }

    if(estatus != QProcess::NormalExit) {
        QString errmsg = QLatin1String("videofile.info crashed");
        errs.append(errmsg);
    }

    if(sout.isEmpty()) {
        // no crop
    }

    else if(rxcs.indexIn(sout) < 0) {
        QString errmsg = QString(QLatin1String("bad cropstring \"%1\"")).arg(sout);
        errs.append(errmsg);
    }

    if(!errs.isEmpty()) {
        if(!serr.isEmpty()) {
            errs.append((serr));
        }

        QString errmsg = errs.join(QChar(QLatin1Char('\n')));
        emit sig_detected(false, errmsg, mfn);
    }
    else {
        emit sig_detected(true, sout, mfn);
    }
}
