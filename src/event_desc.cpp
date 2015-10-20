#include "event_desc.h"
#include "event_types.h"

#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <QLoggingCategory>
#include <5.5.0/QtCore/private/qobject_p.h>
#include <QApplication>
#include <QWidget>
#include <QMetaMethod>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaMethod>


#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "ED"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static_var QHash<long, QString> name_cache;
static_var QMutex name_cache_lock;

static QString cache_lookup_objectname(const QObject *o)
{
    if(o == NULL) {
        return QString();
    }

    const long key = (long)(void *)o;
    QMutexLocker locker(&name_cache_lock);
    return name_cache.value(key);
}
static void cache_set_objectname(const QObject *o, const QString &val)
{
    if(o == NULL) {
        return;
    }

    const long key = (long)(void *)o;
    QMutexLocker locker(&name_cache_lock);
    name_cache.insert(key, val);
}

void calculate_qoname_CPP(QObject *o, QString &oN)
{
    if(o == NULL) {
        oN.clear();

        return;
    }

    oN = o->objectName();

    if(oN.isEmpty()) {
        const QMetaObject *mo = o->metaObject();
        char const *cN = mo->className();

        oN = QLatin1String(cN);

    }

    cache_set_objectname(o, oN);
}

void calculate_qoname_C(QObject *o, QByteArray &printableN)
{
    if(o == NULL) {
        printableN.clear();
        return;
    }

    QString oN = o->objectName();

    if(!oN.isEmpty()) {
        printableN = oN.toLocal8Bit();
    }
    else {
        const QMetaObject *const mo = o->metaObject();
        char const *const cN = mo->className();

        printableN = QByteArray::fromRawData(cN, strlen(cN));
        oN = QLatin1String(cN);
    }

    cache_set_objectname(o, oN);
}

static QString event_desc_qkeyevent(QKeyEvent *event)
{
    QString msg;

    int keyDisplayed = event->key();
    const Qt::Key keys = static_cast<Qt::Key>(keyDisplayed);

    // if the key pressed is only a modifier, we reset the key
    if(keys == Qt::Key_Control) {
        msg.append(QStringLiteral("Ctrl"));
        keyDisplayed = 0;
    }
    else if(keys == Qt::Key_Shift) {
        msg.append(QStringLiteral("Shift"));
        keyDisplayed = 0;
    }
    else if(keys == Qt::Key_Alt) {
        msg.append(QStringLiteral("Alt"));
        keyDisplayed = 0;
    }
    else if(keys == Qt::Key_Meta) {
        msg.append(QStringLiteral("Meta"));
        //msg.append(QKeySequence(Qt::META));
        keyDisplayed = 0;
    }

    QString text = event->text();

    if(!text.isEmpty()) {
        msg.append(QStringLiteral("\""));
        msg.append(text);
        msg.append(QStringLiteral("\""));
    }

    const Qt::KeyboardModifiers modifiers = event->modifiers();

    if(modifiers & Qt::ShiftModifier) {
        keyDisplayed |= Qt::SHIFT;
        msg.append(QStringLiteral("Shift"));
    }

    if(modifiers & Qt::ControlModifier) {
        keyDisplayed |= Qt::CTRL;
        msg.append(QStringLiteral("Ctrl"));
    }

    if(modifiers & Qt::MetaModifier) {
        keyDisplayed |= Qt::META;
        msg.append(QStringLiteral("Meta"));
    }

    if(modifiers & Qt::AltModifier) {
        keyDisplayed |= Qt::ALT;
        msg.append(QStringLiteral("Alt"));
    }

    //msg.append(QKeySequence(keyDisplayed));

    return msg;
}

static QString event_desc_qmetacallevent(QObject *receiver, QMetaCallEvent *event, bool *ignore_this)
{
    QMetaMethod slot = receiver->metaObject()->method(event->id());
    QByteArray slotsignature = slot.methodSignature();

    if(slotsignature == "slot_readStdout()") {
        *ignore_this = true;
        return QString();
    }

    if(slotsignature == "slot_heartbeat()") {
        *ignore_this = true;
        return QString();
    }

    QString ret;
    // FIXME we assume that QMetaMethod.methodSignature is a latin1literal...
    ret.append(QLatin1String(slotsignature.constData()));

    const QObject *sendero = event->sender();

    if(sendero == NULL) {
        ret.append(QStringLiteral(" from nobody/timer"));
        return ret;
    }

    QString oN = cache_lookup_objectname(sendero);

    if(!oN.isEmpty()) {
        ret.append(QStringLiteral(" from "));
        ret.append(oN);
    }
    else {
        ret.append(QStringLiteral(" from unknown"));
    }

    return ret;

}

static_var const char *const focus_reason_to_name_ar[8] = {
    "Mouse",
    "Tab",
    "Backtab",
    "ActiveWindow",
    "Popup",
    "Shortcut",
    "MenuBar",
    "Other",
};

static char const *focus_reason_to_name_latin1lit(Qt::FocusReason e)
{
    const int i = e;

    if(i >= 0 && i < 8) {
        return focus_reason_to_name_ar[i];
    }

    return "UnknownEnum";
}

static QString event_desc_qfocusevent(QFocusEvent *event)
{
    QWidget *w = QApplication::focusWidget();

    QString ret = QStringLiteral("focus currently ");

    if(w == NULL) {
        ret.append(QStringLiteral("nowhere/outside"));
    }
    else {
        QString oN;
        calculate_qoname_CPP(w, oN);
        ret.append(oN);
    }

    const Qt::FocusReason reason = event->reason();

    if(reason != Qt::OtherFocusReason) {
        ret.append(QStringLiteral(" ("));
        ret.append(QLatin1String(focus_reason_to_name_latin1lit(reason)));
        ret.append(QStringLiteral(")"));
    }

    return ret;

}

static QString event_desc_qresizeevent(QResizeEvent *event)
{
    const QSize &os = event->oldSize();
    const QSize &ns = event->size();

    QString ret = QStringLiteral("from ");
    ret.append(QString::number(os.width()));
    ret.append(QLatin1Char('x'));
    ret.append(QString::number(os.height()));
    ret.append(QStringLiteral(" to "));
    ret.append(QString::number(ns.width()));
    ret.append(QLatin1Char('x'));
    ret.append(QString::number(ns.height()));

    return ret;

}

static QString event_desc_qchildevent(QChildEvent *event)
{
    QObject *c = event->child();

    if(c != NULL) {
        QString oN;
        calculate_qoname_CPP(c, oN);
        return oN;
    }

    return QString();

}

static QString event_desc(QObject *receiver, QEvent *event)
{
    const QEvent::Type type = event->type();
    char const *const type_name_latin1lit = event_type_2_name_latin1lit(type);
    QString ret = QLatin1String(type_name_latin1lit);

    QString spec;

    // What MetaCall?
    // text: convert \n to \\n

    if(type == QEvent::KeyPress || type == QEvent::KeyRelease || type == QEvent::ShortcutOverride) {
        spec = event_desc_qkeyevent(dynamic_cast<QKeyEvent *>(event));
    }

    else if(type == QEvent::MetaCall) {
        bool ignore_this = false;
        spec = event_desc_qmetacallevent(receiver, dynamic_cast<QMetaCallEvent *>(event), &ignore_this);

        if(ignore_this) {
            return QString();
        }
    }

    else if(type == QEvent::FocusIn || type == QEvent::FocusOut || type == QEvent::FocusAboutToChange) {
        spec = event_desc_qfocusevent(dynamic_cast<QFocusEvent *>(event));
    }

    else if(type == QEvent::ChildRemoved || type == QEvent::ChildAdded || type == QEvent::ChildPolished) {
        spec = event_desc_qchildevent(dynamic_cast<QChildEvent *>(event));
    }

    else if(type == QEvent::Resize) {
        spec = event_desc_qresizeevent(dynamic_cast<QResizeEvent *>(event));
    }

    if(!spec.isEmpty()) {
        ret.append(QLatin1Char(' '));
        ret.append(spec);
    }

    return ret;
}

static bool ignore_ev(QEvent *event)
{
    const QEvent::Type type = event->type();

    if(type == QEvent::Move) {
        return true;
    }

    if(type == QEvent::HoverMove) {
        return true;
    }

    if(type == QEvent::InputMethodQuery) {
        return true;
    }

    if(type == QEvent::MouseMove) {
        return true;
    }

    if(type == QEvent::Polish) {
        return true;
    }

    if(type == QEvent::PolishRequest) {
        return true;
    }

    if(type == QEvent::ChildPolished) {
        return true;
    }

    return false;
}

void log_qevent(QLoggingCategory const &lcat, QObject *receiver, QEvent *event)
{
    if(ignore_ev(event)) {
        return;
    }

    const QString ed = event_desc(receiver, event);

    if(ed.isEmpty()) {
        return;
    }

    QByteArray printableN;
    calculate_qoname_C(receiver, printableN);

    qCDebug(lcat, "%-20s::event(%s)", printableN.constData(), qPrintable(ed));
}

void log_qeventFilter(QLoggingCategory const &lcat, QObject *receiver, QEvent *event)
{
    if(ignore_ev(event)) {
        return;
    }

    const QString ed = event_desc(receiver, event);

    if(ed.isEmpty()) {
        return;
    }

    QByteArray printableN;
    calculate_qoname_C(receiver, printableN);

    qCDebug(lcat, "%-20s::evenF(%s)", printableN.constData(), qPrintable(ed));
}

static_var bool in_sbc = false;
static_var QMutex in_sbc_lock;

void signal_begin_callback(QObject *caller, int signal_or_method_index, void **argv)
{
    if(caller == NULL) {
        return;
    }

    const QMetaObject *mo = caller->metaObject();
    char const *cN = mo->className();

    // these spew a lot of uninteresting signals, ignoring them
    if(strcmp(cN, "QEventDispatcherUNIX") == 0) {
        return;
    }

    if(strcmp(cN, "QXcbEventReader") == 0) {
        return;
    }

    if(strcmp(cN, "QUnixEventDispatcherQPA") == 0) {
        return;
    }

    {
        // the lock serializes checking of the in_sbc flag
        QMutexLocker locker(&in_sbc_lock);

        // this flag makes sure we do not get an infinite recursion
        if(in_sbc) {
            return;
        }

        in_sbc = true;

        // I'm not sure this gives us the correct method
        const QMetaMethod method = mo->method(signal_or_method_index);
        const QMetaMethod::MethodType mtype = method.methodType();

        if(mtype != QMetaMethod::Signal) {
            QByteArray printableN;
            calculate_qoname_C(caller, printableN);
            qWarning("WTF? %s sent signal %d, but returned method %s has a type of %d not %d(signal)"
                     , cN
                     , signal_or_method_index
                     , method.name().constData()
                     , mtype
                     , QMetaMethod::Signal
                    );
            in_sbc = false;
            return;
        }

        const QByteArray name = method.name();

        if(name == "deleteLater") {
            in_sbc = false;
            return;
        }

        QByteArray printableN;
        calculate_qoname_C(caller, printableN);

        const QList<QByteArray> params = method.parameterTypes();
        QString paramdesc(QStringLiteral("("));

        if(params.size() > 0) {

            for(int i = 0; i < params.count(); ++i) {
                char const *typedesc = params.at(i).constData();
                const int tp = QMetaType::type(typedesc);

                if(tp == QMetaType::Void) {
                    qWarning("Don't know how to handle '%s', use qRegisterMetaType to register it.", typedesc);
                    continue;
                }

                const QMetaType::Type type = static_cast<QMetaType::Type>(tp);
                const QVariant val(type, argv[i + 1]);

                if(val.canConvert<QString>()) {
                    const QString vals = val.toString();
                    paramdesc.append(vals);
                }
                else {
                    paramdesc.append(QLatin1String(typedesc));
                }

                if(i < params.count() - 1) {
                    paramdesc.append(QLatin1String(", "));
                }

            }
        }

        paramdesc.append(QLatin1String(")"));

        if(printableN == "QFile" || printableN == "QFileDevice") {
            const QString fN = dynamic_cast<QFileDevice *>(caller)->fileName();

            if(fN.isEmpty()) {
                MYDBG("%s SIGNAL %s%s", printableN.constData(), name.constData(), qPrintable(paramdesc));
            }
            else {
                MYDBG("%s(%s) SIGNAL %s%s", printableN.constData(), qPrintable(fN), name.constData(), qPrintable(paramdesc));
            }
        }

        MYDBG("%s SIGNAL %s%s", printableN.constData(), name.constData(), qPrintable(paramdesc));

        in_sbc = false;
    }
}


QSignalSpyCallbackSet my_signal_spy_callback_set = { signal_begin_callback, 0, 0, 0 };

void init_signals_spy()
{
    // from qobject_p.h
    // qt_register_signal_spy_callbacks(my_signal_spy_callback_set);

    // this seems pretty expensive, and I'm not sure I'm printing the correct
    // signal name, so disabling for now

}
