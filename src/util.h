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

#define INVOKE_DELAY(X)    QTimer::singleShot(0, this, SLOT(X));
#define QUEUEDCONN         Qt::QueuedConnection
#define xinvokeMethod(...) do{ \
       if(!QMetaObject::invokeMethod(__VA_ARGS__)){\
          PROGRAMMERERROR("method call failed");\
       }\
    } while(0)
#define XCONNECT(...)      do{ \
       if(!QObject::connect(__VA_ARGS__)){\
          PROGRAMMERERROR("connect failed");\
       }\
    } while(0)

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
    QChar other = QChar::fromAscii(beg);

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

inline QString xbin_2_codec_qstring(bool doerr, char const *const blob, const size_t len, QTextCodec const *const codec)
{
    if(blob == NULL) {
        PROGRAMMERERROR("NULL blob-string?");
    }

    if(len == 0) {
        return QString();
    }

    if(codec == NULL) {
        qFatal("Blob-string \"%s\": no codec found?", blob);
    }

    QTextCodec::ConverterState state;
    const QString string = codec->toUnicode(blob, len, &state);

    if(state.invalidChars > 0) {
        if(doerr) {
            qFatal("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
        else {
            qWarning("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }

    }

    if(state.remainingChars > 0) {
        if(doerr) {
            qFatal("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
        else {
            qWarning("Blob-string \"%s\" not a valid %s sequence.", blob, codec->name().constData());
        }
    }

    return string;
}


inline QString warn_xbin_2_local_qstring(char const *const blob)
{
    if(blob == NULL) {
        PROGRAMMERERROR("NULL blob-string?");
    }

    const size_t len = strlen(blob);
    return xbin_2_codec_qstring(false, blob, len, QTextCodec::codecForLocale());
}

inline QString err_xbin_2_local_qstring(char const *const blob)
{
    if(blob == NULL) {
        PROGRAMMERERROR("NULL blob-string?");
    }

    const size_t len = strlen(blob);
    return xbin_2_codec_qstring(true, blob, len, QTextCodec::codecForLocale());
}

inline QString err_xbin_2_utf8_qstring(char const *const blob)
{
    if(blob == NULL) {
        PROGRAMMERERROR("NULL blob-string?");
    }

    const size_t len = strlen(blob);
    return xbin_2_codec_qstring(true, blob, len, QTextCodec::codecForName("UTF-8"));
}

inline QString warn_xbin_2_local_qstring(const QByteArray &vblob)
{
    return xbin_2_codec_qstring(false, vblob.constData(), vblob.size(), QTextCodec::codecForLocale());
}

inline QString err_xbin_2_local_qstring(const QByteArray &vblob)
{
    return xbin_2_codec_qstring(true, vblob.constData(), vblob.size(), QTextCodec::codecForLocale());
}

bool definitely_running_from_desktop();
void desktopMessageOutput(QtMsgType type, const char *msg);
void commandlineMessageOutput(QtMsgType type, const char *msg);

QString get_MP_VO();



#endif // UTIL_H
