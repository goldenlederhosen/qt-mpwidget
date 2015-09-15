#ifndef UTIL_H
#define UTIL_H

#include <QLatin1String>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QTextCodec>
#include <QHash>
#include <QSet>
#include <QMetaType>

#include <valgrind/valgrind.h>

#define PROGRAMMERERROR(msg, ...) do { \
        VALGRIND_PRINTF_BACKTRACE("PROGRAMMERERROR at %s:%lu: " msg "\n", __FILE__, (unsigned long) __LINE__, ##__VA_ARGS__);\
        qFatal("PROGRAMMERERROR at %s:%lu: " msg, __FILE__, (unsigned long) __LINE__, ##__VA_ARGS__); \
    } while(0);

#define ALLDBG(msg, ...) qWarning("%s:%lu: " msg, __FILE__, (unsigned long) __LINE__, ##__VA_ARGS__)

enum class NoYesUnknown {
    No,
    Yes,
    Unknown
};

enum class Interlaced_t {
    Interlaced,
    Progressive,
    PIUnknown
};

typedef QHash<QString, QString> BadMovieRecord_t;

Q_DECLARE_METATYPE(BadMovieRecord_t)

typedef QHash<QString, QSet<QString> > RdirRmovfns_t;

static inline bool string_starts_with_remove(QString &str, char const *const beglatin1)
{
    // warning string alloc?
    if(str.startsWith(QLatin1String(beglatin1))) {
        const size_t sublen = strlen(beglatin1);
        str.remove(0, sublen);
        return true;
    }

    return false;
}

static inline bool string_starts_with_remove(QString &str, char beg)
{
    QChar other = QChar::fromLatin1(beg);

    if(str[0] == other) {
        str.remove(0, 1);
        return true;
    }

    return false;
}

// untested
static inline bool string_starts_with_remove(char *str, char const *const beg)
{
    const size_t sublen = strlen(beg);

    if(0 == strncmp(str, beg, sublen)) {
        const size_t len = strlen(str);
        memmove(str, str + sublen, len - sublen);
        return true;
    }

    return false;
}

QString dt_2_human(const QDateTime &lastseenon);
QString tlenpos_2_human(double sec);

QStringList doSplitArgs(const QString &args);

// false - not set or not "1"
// true - set and 1
bool setand1_getenv(char const *const varname);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
void desktopMessageOutput(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
void commandlineMessageOutput(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
#else
void desktopMessageOutput(QtMsgType type, const char *msg);
void commandlineMessageOutput(QtMsgType type, const char *msg);
#endif

class QWidget;
void set_focus_raise(QWidget *w);
QString widget_2_name(QWidget *w);



#endif // UTIL_H
