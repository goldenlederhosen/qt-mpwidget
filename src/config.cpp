#include "config.h"
#include "encoding.h"

#include <QDir>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "CFG"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static QString get_X_core(char const *const key_c_latin1lit)
{
    const QByteArray envval = qgetenv(key_c_latin1lit);

    if(!envval.isEmpty()) {
        MYDBG("got %s=%s from environment variable", key_c_latin1lit, envval.constData());
        return err_xbin_2_local_qstring(envval);
    }

    QSettings settings;
    const QString key = QLatin1String(key_c_latin1lit);

    if(settings.contains(key)) {
        const QString ret = settings.value(key).toString();
        MYDBG("got %s=%s from settings", key_c_latin1lit, qPrintable(ret));
        return ret;
    }

    MYDBG("could not find a value for %s in environment or settings", key_c_latin1lit);
    QString empty;
    return empty;
}

static QString get_X(char const *const key_c_latin1lit, QString *cache, QMutex *mutex)
{
    {
        QMutexLocker l(mutex);

        if(!cache->isEmpty()) {
            QString copy = *cache;
            copy.detach();
            return copy;
        }
    }

    const QString val = get_X_core(key_c_latin1lit);

    {
        QMutexLocker l(mutex);
        *cache = val;
        cache->detach();
    }

    return val;
}

static_var QString cache_MP_VO;
static_var QMutex mutex_MP_VO;

QString get_MP_VO()
{
    return get_X("MP_VO", &cache_MP_VO, &mutex_MP_VO);
}

static_var QString cache_MP_OPTS_APPEND;
static_var QMutex mutex_MP_OPTS_APPEND;

QString get_MP_OPTS_APPEND()
{
    return get_X("MP_OPTS_APPEND", &cache_MP_OPTS_APPEND, &mutex_MP_OPTS_APPEND);
}

static_var QString cache_MP_OPTS_OVERRIDE;
static_var QMutex mutex_MP_OPTS_OVERRIDE;

QString get_MP_OPTS_OVERRIDE()
{
    return get_X("MP_OPTS_OVERRIDE", &cache_MP_OPTS_OVERRIDE, &mutex_MP_OPTS_OVERRIDE);
}

static_var QString cache_VDB_RUN;
static_var QMutex mutex_VDB_RUN;

QString get_VDB_RUN()
{
    return get_X("VDB_RUN", &cache_VDB_RUN, &mutex_VDB_RUN);
}
