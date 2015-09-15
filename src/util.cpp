#include <sys/types.h>
#include <unistd.h>
#include <QDebug>
#include <QApplication>
#include <QProcess>
#include <QWidget>

#include "util.h"
#include "gui_overlayquit.h"
#include "encoding.h"
#include "safe_signals.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "UTIL"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

QString dt_2_human(const QDateTime &lastseenon)
{
    QString ds;
    QDateTime now = QDateTime::currentDateTime();
    QDate nowd = now.date();
    QDate lsnd = lastseenon.date();
    const int daysago = lsnd.daysTo(nowd);

    if(daysago == 0) {
        ds = QLatin1String("today");
    }
    else if(daysago == 1) {
        ds = QLatin1String("yesterday");
    }
    else if(daysago < 6) {
        ds = lastseenon.toString(QLatin1String("dddd"));
    }
    else if(nowd.year() != lsnd.year()) {
        ds = lastseenon.toString(QLatin1String("d MMMM yyyy"));
    }
    else if(nowd.month() != lsnd.month()) {
        ds = lastseenon.toString(QLatin1String("d MMMM"));
    }
    else if(nowd.day() != lsnd.day()) {
        ds = lastseenon.toString(QLatin1String("dddd d MMMM"));
    }
    else {
        ds = QLatin1String("today");
    }

    return ds;
}

QString tlenpos_2_human(double dsec)
{
    if(dsec > 60 * 60 * 24 * 10) { // 10 days
        PROGRAMMERERROR("time length %f sec much too large", dsec);
    }

    if(dsec < 0) {
        PROGRAMMERERROR("time length %f sec negative", dsec);
    }

    double rsecs = round(dsec);

    if(rsecs < 1) {
        return QLatin1String("00:00:00");
    }

    if(rsecs >= SIZE_MAX) {
        return QLatin1String("99:99:99");
    }

    size_t secs = rsecs;
    size_t mins = secs / 60;
    secs -= mins * 60;
    size_t hours = mins / 60;
    mins -= hours * 60;
    size_t days = hours / 24;
    hours -= days * 24;

    QString ts;
    ts.sprintf("%02u:%02u:%02u", unsigned(hours), unsigned(mins), unsigned(secs));

    if(days > 10) {
        PROGRAMMERERROR("WTF");
    }

    if(days > 0) {
        ts.prepend(QString::number(days) + QLatin1String(" days "));
    }

    return ts;
}

static inline bool isWhiteSpace(ushort c)
{
    return c == ' ' || c == '\t';
}

QStringList doSplitArgs(const QString &args)
{
    QStringList ret;

    int p = 0;
    const int length = args.length();

    forever {
        forever {
            if(p == length) {
                return ret;
            }

            if(!isWhiteSpace(args.unicode()[p].unicode())) {
                break;
            }

            ++p;
        }

        QString arg;
        bool inquote = false;

        forever {
            bool copy = true; // copy this char
            int bslashes = 0; // number of preceding backslashes to insert

            while(p < length && args.unicode()[p] == QLatin1Char('\\')) {
                ++p;
                ++bslashes;
            }

            if(p < length && args.unicode()[p] == QLatin1Char('"')) {
                if(!(bslashes & 1)) {
                    // Even number of backslashes, so the quote is not escaped.
                    if(inquote) {
                        if(p + 1 < length && args.unicode()[p + 1] == QLatin1Char('"')) {
                            // This is not documented on MSDN.
                            // Two consecutive quotes make a literal quote. Brain damage:
                            // this still closes the quoting, so a 3rd quote is required,
                            // which makes the runtime's quoting run out of sync with the
                            // shell's one unless the 2nd quote is escaped.
                            ++p;
                        }
                        else {
                            // Closing quote
                            copy = false;
                        }

                        inquote = false;
                    }
                    else {
                        // Opening quote
                        copy = false;
                        inquote = true;
                    }
                }

                bslashes >>= 1;
            }

            while(--bslashes >= 0) {
                arg.append(QLatin1Char('\\'));
            }

            if(p == length || (!inquote && isWhiteSpace(args.unicode()[p].unicode()))) {
                ret.append(arg);

                if(inquote) {
                    return QStringList();
                }

                break;
            }

            if(copy) {
                arg.append(args.unicode()[p]);
            }

            ++p;
        }
    }

    //not reached
}

bool setand1_getenv(char const *const varname)
{
    const QByteArray envval = qgetenv(varname);

    if(envval.isEmpty()) {
        MYDBG("setand1_getenv(%s): not set", varname);
        return false;
    }

    if(envval.size() != 1) {
        MYDBG("setand1_getenv(%s): \"%s\" not 1", varname, envval.constData());
        return false;
    }

    if(envval.at(0) != '1') {
        MYDBG("setand1_getenv(%s): \"%s\" not 1", varname, envval.constData());
        return false;
    }

    MYDBG("setand1_getenv(%s): exactly 1", varname);
    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

void desktopMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &qmsg)
{

#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
    const QByteArray bmsg = qFormatLogMessage(type, context, qmsg).toLocal8Bit();
#else
    Q_UNUSED(context);
    const QByteArray bmsg = qmsg.toLocal8Bit();
#endif

    const char *msg = bmsg.constData();

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warning: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = QApplication::focusWidget();

            if(mainw != NULL) {
                QLatin1String format("Critical: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, "OQ_crit", qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Critical: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = QApplication::focusWidget();

            if(mainw != NULL) {
                QLatin1String format("Fatal: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, "OQ_fatal", qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}
void commandlineMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &qmsg)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
    const QByteArray bmsg = qFormatLogMessage(type, context, qmsg).toLocal8Bit();
#else
    Q_UNUSED(context);
    const QByteArray bmsg = qmsg.toLocal8Bit();
#endif
    const char *msg = bmsg.constData();

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warni: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            fprintf(stderr, "Criti: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}


#else


void desktopMessageOutput(QtMsgType type, const char *msg)
{
    //const bool isPE=( strstr(msg,"PROGRAMMERERROR")!=NULL );

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warning: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = QApplication::focusWidget();

            if(mainw != NULL) {
                QLatin1String format("Critical: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, "OQ_crit", qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Critical: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = QApplication::focusWidget();

            if(mainw != NULL) {
                QLatin1String format("Fatal: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, "OQ_fatal", qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}
void commandlineMessageOutput(QtMsgType type, const char *msg)
{
    //const bool isPE=( strstr(msg,"PROGRAMMERERROR")!=NULL );

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warni: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            fprintf(stderr, "Criti: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}

#endif // QT4

void set_focus_raise(QWidget *w)
{
    w->show();
    w->raise();
    w->setFocus(Qt::OtherFocusReason);
    MYDBG("giving focus to %s", qPrintable(w->objectName()));
}

QString widget_2_name(QWidget *w)
{
    if(w == NULL) {
        return QLatin1String("<none>");
    }

    QString o = w->objectName();

    if(o.isEmpty()) {
        return QLatin1String("<unnamed>");
    }

    return o;
}
