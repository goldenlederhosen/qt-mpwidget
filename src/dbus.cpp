#include "dbus.h"

#include <QDBusConnection>
#include <QString>
#include <QVariant>
#include <QStringList>

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "DBUS"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static QString QVariant_to_string(const QVariant &v)
{
    const QString s = v.toString();
    char const *const typen = v.typeName();
    const QString types = QLatin1String(typen);
    const QString ret =  types + QLatin1Char(':') + s;
    return ret;
}

static char const *QDbusMessageType_2_desc(const QDBusMessage::MessageType &t)
{
    switch(t) {
        case QDBusMessage::MethodCallMessage:
            return "method";

        case QDBusMessage::SignalMessage:
            return "signal";

        case QDBusMessage::ReplyMessage:
            return "reply";

        case QDBusMessage::ErrorMessage:
            return "error";

        case QDBusMessage::InvalidMessage:
            return "invalidmessage";
    }

    return "unknownmessagetype";
}

static QString QDBusMessage_argdesc(const QDBusMessage &m)
{
    const  QList<QVariant> args = m.arguments();
    QStringList argss;

    foreach(const QVariant &v, args) {
        const QString s = QVariant_to_string(v);
        argss.append(s);
    }

    const QString args_desc = argss.join(QLatin1String(", "));
    return args_desc;
}

static QString QDBusMessage_desc(const QDBusMessage &m)
{
    const QString types = QLatin1String(QDbusMessageType_2_desc(m.type()));
    const QString service = m.service();
    const QString path = m.path();
    const QString interface = m.interface();
    const QString method = m.member();
    const QString args = QDBusMessage_argdesc(m);
    const QString errdesc = m.errorMessage();
    const QString errname = m.errorName();
    // org.freedesktop.DBus.Error.UnknownInterface if interface does not exist
    // org.freedesktop.DBus.Error.ServiceUnknown service not found
    // org.freedesktop.DBus.Error.UnknownObject no such path
    // org.freedesktop.DBus.Error.UnknownMethod no such method or wrong arguments

    QString ret = types;
    ret += QLatin1String(" ") + service;
    ret += path;

    if(!ret.isEmpty() && !interface.isEmpty()) {
        ret += QLatin1String(":");
    }

    ret += interface;

    if(!ret.isEmpty() && !method.isEmpty()) {
        ret += QLatin1String(".");
    }

    ret += method;

    if(!ret.isEmpty() && !args.isEmpty()) {
        ret += QLatin1Char('(') ;
    }

    ret += args;

    if(!ret.isEmpty() && !args.isEmpty()) {
        ret += QLatin1Char(')');
    }

    if(!ret.isEmpty() && !errname.isEmpty()) {
        ret += QLatin1String(" ");
    }

    ret += errname;

    if(!ret.isEmpty() && !errdesc.isEmpty()) {
        ret += QLatin1String(" ");
    }

    ret += errdesc;

    return ret;

}

static bool is_bad_error(const QDBusMessage &m)
{
    const QDBusMessage::MessageType t = m.type();

    switch(t) {
        case QDBusMessage::MethodCallMessage:
            return false;

        case QDBusMessage::SignalMessage:
            return false;

        case QDBusMessage::ReplyMessage:
            return false;

        case QDBusMessage::ErrorMessage: {
            const QString errname = m.errorName();

            // if we can not find the interface that is fine
            if(errname == QLatin1String("org.freedesktop.DBus.Error.UnknownInterface")) {
                return false;
            }
            else {
                return true;
            }

        }

        case QDBusMessage::InvalidMessage:
            return true;
    }

    // we did not catch that case, definitely bad
    return true;

}

QDBusMessage verbose_dbus(const QDBusMessage &m)
{
    MYDBG("calling %s", qPrintable(QDBusMessage_desc(m)));
    QDBusMessage response = QDBusConnection::sessionBus().call(m);

    const bool bad = is_bad_error(response);

    if(bad) {
        qWarning("D-BUS call %s failed:\n     %s", qPrintable(QDBusMessage_desc(m)), qPrintable(QDBusMessage_desc(response)));
    }
    else {
        MYDBG("    returned %s", qPrintable(QDBusMessage_desc(response)));
    }

    return response;
}

