#include "logging.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <QByteArray>
#include <QLoggingCategory>
#include <QDebug>
#include <QString>
#include <QMutex>
#include <QMutexLocker>
#include <QApplication>
#include <QDateTime>

#include "util.h"
#include "gui_overlayquit.h"
#include "encoding.h"
#include "system.h"
#include "safe_signals.h"
#include "circularbuffer.h"

#define THIS_SOURCE_FILE_LOG_CATEGORY "LOG"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static_var QDateTime *starttime = NULL;

template<typename T>
inline T MIN(const T a, const T b)
{
    if(a < b) {
        return a;
    }
    else {
        return b;
    }
}

static unsigned long calculate_msec_since_start(const QDateTime &now)
{
    const qint64 msec_s = starttime->msecsTo(now);
    unsigned long msec_since_start = msec_s < 0 ? 0 : msec_s;
    return msec_since_start;
}

static unsigned long calculate_now_msec_since_start()
{
    return calculate_msec_since_start(QDateTime::currentDateTime());
}

class SerializedMsg
{
private:
    QtMsgType m_type;
    int m_line;
    int m_version;
    int m_size;
    unsigned long m_msec_since_start;
    long m_tid;
    char m_category[64];
    char m_file[64];
    char m_function[128];
    QChar m_unicode[512];

public:
    // this needs to be fast
    SerializedMsg(QtMsgType in_type
                  , const QMessageLogContext &in_context
                  , const QString &in_msg
                  , const unsigned long in_msec_since_start
                  , const long in_tid
                 )
        : m_type(in_type)
        , m_line(in_context.line)
        , m_version(in_context.version)
        , m_size(MIN(in_msg.size(), 512))
        , m_msec_since_start(in_msec_since_start)
        , m_tid(in_tid)
    {
        if(in_context.category != NULL) {
            strncpy(m_category, in_context.category, 64 - 1);
            m_category[64 - 1] = '\0';
        }
        else {
            m_category[0] = '\0';
        }

        if(in_context.file != NULL) {
            strncpy(m_file, in_context.file, 64 - 1);
            m_file[64 - 1] = '\0';
        }
        else {
            m_file[0] = '\0';
        }

        if(in_context.function != NULL) {
            strncpy(m_function, in_context.function, 128 - 1);
            m_function[128 - 1] = '\0';
        }
        else {
            m_function[0] = '\0';
        }

        memcpy(m_unicode, in_msg.unicode(), m_size * sizeof(QChar));
    }
    QtMsgType type() const
    {
        return m_type;
    }
    void set_qmsg(QString &msg) const
    {
        msg.setUnicode(m_unicode, m_size);
    }
    char const *category() const
    {
        return m_category;
    }
    unsigned long msec_since_start() const
    {
        return m_msec_since_start;
    }
    long tid() const
    {
        return m_tid;
    }


};

static_var MyCircularBuffer<SerializedMsg> *dbgrecbuf = NULL;
static_var QMutex dbgrecbuflock;

static_var QLoggingCategory::CategoryFilter oldCategoryFilter;
static_var QMutex LogInitLock;

static_var const pid_t PID = getpid();

static_var const bool print_all_unconditionally = setand1_getenv("MC_ALL_LOG");

static char const *type_2_str(QtMsgType type)
{
    switch(type) {
        case QtDebugMsg: {
            return "Debug";
        }
        break;

        case QtInfoMsg: {
            return "Info ";
        }
        break;

        case QtWarningMsg: {
            return "Warni";
        }
        break;

        case QtCriticalMsg: {
            return "Criti";
        }
        break;

        case QtFatalMsg: {
            return "Fatal";
        }
        break;

        default: {
            return "Unkno";
        }
        break;
    }
}

static void MessageOutput_core(const unsigned long msec, QtMsgType type, const QString &qmsg, char const *category, const long tid)
{

    /*
    #if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
        const QByteArray bmsg = qFormatLogMessage(type, context, qmsg).toLocal8Bit();
    #else
        Q_UNUSED(context);
        const QByteArray bmsg = qmsg.toLocal8Bit();
    #endif
    */

    QByteArray bprefix(type_2_str(type));
    bprefix.append(": ");
    bprefix.append(QByteArray::number(qulonglong(msec)).rightJustified(6));

    if(PID != tid) {
        bprefix.append(" [");
        bprefix.append(QByteArray::number(qulonglong(PID)));
        bprefix.append("|");
        bprefix.append(QByteArray::number(qulonglong(tid)));
        bprefix.append("]");
    }

    bprefix.append(" ");

    if(strcmp(category, "default") == 0) {
        bprefix.append("         ");
    }
    else {
        // FIXME we assume category is a latin1 literal
        QByteArray bcategory(category);

        if(bcategory.size() < 7) {
            bcategory = bcategory.leftJustified(7, ' ');
        }

        bprefix.append(bcategory);
        bprefix.append(": ");
    }

    if(bprefix.size() < 20) {
        bprefix = bprefix.leftJustified(20, ' ');
    }

    QString formated_msg = qmsg;
    formated_msg.replace(QLatin1Char('\r'), QLatin1String("\\r"));
    formated_msg.replace(QLatin1Char('\a'), QLatin1String("\\a"));
    formated_msg.replace(QLatin1Char('\b'), QLatin1String("\\b"));
    formated_msg.replace(QLatin1Char('\f'), QLatin1String("\\f"));
    formated_msg.replace(QLatin1Char('\t'), QLatin1String("\\t"));
    formated_msg.replace(QLatin1Char('\v'), QLatin1String("\\v"));
    formated_msg.replace(QLatin1Char('\0'), QLatin1String("\\0"));
    formated_msg.replace(QLatin1Char('\e'), QLatin1String("\\e"));

    if(formated_msg.contains(QLatin1Char('\n'))) {
        QByteArray NLbprefix = bprefix;
        NLbprefix.prepend('\n');
        QLatin1String LNLbprefix(NLbprefix);
        formated_msg.replace(QLatin1Char('\n'), LNLbprefix);
    }

    formated_msg.prepend(QLatin1String(bprefix));

    QByteArray byformated_msg = formated_msg.toUtf8();

    const char *cformated_msg = byformated_msg.constData();

    fprintf(stderr, "%s\n", cformated_msg);

    switch(type) {
        case QtCriticalMsg: {
            if(definitely_running_from_desktop()) {
                QWidget *mainw = QApplication::activeWindow();

                if(mainw != NULL) {
                    QString guitext = QLatin1String("Critical: ");
                    guitext.append(qmsg);
                    OverlayQuit *oq = new OverlayQuit(mainw, "OQ_crit", guitext);
                    XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                    oq->show_and_take_focus(NULL);
                }
            }
        }
        break;

        case QtFatalMsg: {
            if(definitely_running_from_desktop()) {
                QWidget *mainw = QApplication::activeWindow();

                if(mainw != NULL) {
                    QString guitext = QLatin1String("Fatal: ");
                    guitext.append(qmsg);
                    OverlayQuit *oq = new OverlayQuit(mainw, "OQ_fatal", guitext);
                    XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                    oq->show_and_take_focus(NULL);
                }
            }
        }
        break;

        default: {
        }
        break;
    }
}

static void flush_msgbuffer()
{
    QMutexLocker locker(&dbgrecbuflock);

    if(dbgrecbuf->isEmpty()) {
        return;
    }

    size_t sz;
    SerializedMsg const *pt;
    dbgrecbuf->get_range_1(&pt, &sz);

    for(size_t i = 0; i < sz; i++) {
        SerializedMsg const *sm = pt + i;
        QString qmsg;
        sm->set_qmsg(qmsg);
        MessageOutput_core(sm->msec_since_start(), sm->type(), qmsg, sm->category(), sm->tid());

    }

    dbgrecbuf->get_range_2(&pt, &sz);

    for(size_t i = 0; i < sz; i++) {
        SerializedMsg const *sm = pt + i;
        QString qmsg;
        sm->set_qmsg(qmsg);
        MessageOutput_core(sm->msec_since_start(), sm->type(), qmsg, sm->category(), sm->tid());
    }


    dbgrecbuf->clear();
}

static void MessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &ori_qmsg)
{

    init_logging();

    if(type == QtDebugMsg && strcmp(context.category, "qt.qpa.input") == 0) {
        return;
    }

    if(type == QtDebugMsg && strcmp(context.category, "qt.widgets.gestures") == 0) {
        return;
    }

    QString qmsg = ori_qmsg;

    while(qmsg.endsWith(QLatin1Char('\n'))) {
        qmsg.chop(1);
    }

    if(qmsg.isEmpty()) {
        return;
    }

    const long tid = syscall(SYS_gettid);
    const unsigned long msec = calculate_now_msec_since_start();

    if(type == QtWarningMsg || type == QtCriticalMsg) {
        flush_msgbuffer();

        MessageOutput_core(msec, type, qmsg, context.category, tid);

    }
    else if(type == QtFatalMsg) {

        // FIXME we assume file is a latin1 literal
        QString qmsg_with_fl = QLatin1String(context.file);
        qmsg_with_fl.append(QLatin1Char(':'));
        qmsg_with_fl.append(QString::number(context.line));
        qmsg_with_fl.append(QLatin1Char(' '));
        qmsg_with_fl.append(qmsg);
        char *cmsg = strdup(qPrintable(qmsg_with_fl));

        flush_msgbuffer();

        MessageOutput_core(msec, type, qmsg_with_fl, context.category, tid);

        VALGRIND_PRINTF_BACKTRACE("%s\n", cmsg);
        fprintf(stderr, "throwing exception \"%s\"\n", cmsg);
        throw std::runtime_error(cmsg);
    }
    else { // info or debug

        if(print_all_unconditionally) {
            // this needs to be the second fastest path
            MessageOutput_core(msec, type, qmsg, context.category, tid);
        }
        else {
            // this needs to be the fast path
            SerializedMsg r(type, context, qmsg, msec, tid);
            QMutexLocker locker(&dbgrecbuflock);
            dbgrecbuf->append(&r);
        }
    }

}

static void myCategoryFilter(QLoggingCategory *category)
{
    category->setEnabled(QtDebugMsg, true);
}

void init_logging()
{
    {

        QMutexLocker locker(&LogInitLock);

        if(starttime != NULL) {
            return;
        }

        starttime = new QDateTime(QDateTime::currentDateTime());
        dbgrecbuf = new MyCircularBuffer<SerializedMsg>(10000);

    }

    qSetMessagePattern(QStringLiteral("%{if-category}%{category}: %{endif}%{message}"));

    // override with QT_LOGGING_RULES
    QLoggingCategory::setFilterRules(QStringLiteral(
                                         "*.debug=false\n"
                                         "BADLOG=true\n"
                                         "CAPSLOCK=true\n"
                                         "CFG=true\n"
                                         "DBUS=true\n"
                                         "ENCODING=true\n"
                                         "EC=true\n"
                                         "IV=true\n"
                                         "OQ=true\n"
                                         "SL=true\n"
                                         "MAIN=true\n"
                                         "MMPIMP=true\n"
                                         "REMLOC=true\n"
                                         "SSMN=true\n"
                                         "SYS=true\n"
                                         "UTIL=true\n"
                                     ));


    oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
    qInstallMessageHandler(MessageOutput);

    MYDBG("started at %s", qPrintable(starttime->toLocalTime().toString(QStringLiteral("hh:mm:ss.zzz"))));

}

class IL
{
public:
    IL()
    {
        init_logging();
    }
};


static_var IL il;
