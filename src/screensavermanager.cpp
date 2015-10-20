#include "screensavermanager.h"
#include "dbus.h"
#include "xsetscreensaver.h"
#include "singleqprocesssingleshot.h"
#include "event_desc.h"

#include <QTimer>
#include <QDBusMessage>
#include <QApplication>

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "SSMN"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

// when we have the screensaver disabled, simulate activity every N seconds (the Inhibit interface does not always work)
static_var const int screensaver_simulate_activity_every_seconds = 30;

static_var const int timeout_xscreensaver_command_msec = 3000;

ScreenSaverManager::ScreenSaverManager(QObject *parent, predicate_t func, void *payload):
    super(),
    m_screensaver_currently_enabled(NoYesUnknown::Unknown),
    m_sscookie(0),
    m_pwcookie(0),
    m_sscookie_valid(false),
    m_pwcookie_valid(false),
    m_screensaver_should_be_active(func),
    m_screensaver_should_be_active_payload(payload),
    m_found_own_xscreensaver(NoYesUnknown::Unknown)
{
    setObjectName(QStringLiteral("ScreenSaverManager"));
    setParent(parent);
}

ScreenSaverManager::~ScreenSaverManager()
{
    enable();
}

bool ScreenSaverManager::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}

bool ScreenSaverManager::screensaver_should_be_active() const
{
    return (*m_screensaver_should_be_active)(m_screensaver_should_be_active_payload);
}

static NoYesUnknown find_own_xscreensaver()
{
    // xscreensaver-command -version
    const QString exe = QStringLiteral("xscreensaver-command");
    QStringList xcargs;
    xcargs << QStringLiteral("-version");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_xscreensaver-command-version");
    QString error;

    const bool succ = sqpss.run(exe, xcargs, timeout_xscreensaver_command_msec, error);

    if(succ) {
        MYDBG("found xscreensaver: %s %s", qPrintable(sqpss.get_output().trimmed()), qPrintable(sqpss.get_errstr().trimmed()));
        return NoYesUnknown::Yes;
    }
    else {
        MYDBG("could not find xscreensaver: %s", qPrintable(error.trimmed()));
        return NoYesUnknown::No;
    }

}

// pretend that there has just been user activity
void ScreenSaverManager::xscreensaver_command_deactivate()
{
    if(m_found_own_xscreensaver == NoYesUnknown::Unknown) {
        m_found_own_xscreensaver = find_own_xscreensaver();
    }

    if(m_found_own_xscreensaver == NoYesUnknown::No) {
        return;
    }

    const QString exe = QStringLiteral("xscreensaver-command");
    QStringList xcargs;
    xcargs << QStringLiteral("-deactivate");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_xscreensaver-command-deactivate");
    QString error;

    const bool succ = sqpss.run(exe, xcargs, timeout_xscreensaver_command_msec, error);

    if(succ) {
        MYDBG("xscreensaver-command -deactivate successfull: %s %s", qPrintable(sqpss.get_output().trimmed()), qPrintable(sqpss.get_errstr().trimmed()));
    }
    else {
        MYDBG("xscreensaver-command -deactivate unsuccessfull: %s", qPrintable(error.trimmed()));
    }
}

void ScreenSaverManager::slot_screensaver_simulate_activity()
{
    if(screensaver_should_be_active()) {
        MYDBG("slot_screensaver_simulate_activity: screensaver should be active, exiting");
        return;
    }

    MYDBG("slot_screensaver_simulate_activity: screensaver should be inhibited, simulating and re-raising");

    {
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("/ScreenSaver"),
                         QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("SimulateUserActivity"));

        (void) verbose_dbus(m);
    }

    xscreensaver_command_deactivate();

    MYDBG("scheduling slot_screensaver_simulate_activity in %d sec", screensaver_simulate_activity_every_seconds);
    QTimer::singleShot(1000 * screensaver_simulate_activity_every_seconds, this, SLOT(slot_screensaver_simulate_activity()));
}


void ScreenSaverManager::enable()
{
    if(m_screensaver_currently_enabled == NoYesUnknown::Yes) {
        return;
    }

    MYDBG("enabling screensaver");

    xsetscreensaver_enable();

    // FIXME: org.xfce.PowerManager ?
    // org.freedesktop.PowerManagement /org/freedesktop/PowerManagement/Inhibit Inhibit bla bla
    // org.freedesktop.PowerManagement /org/freedesktop/PowerManagement/Inhibit UnInhibit cookie

    if(m_pwcookie_valid) {
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement"),
                         QStringLiteral("/org/freedesktop/PowerManagement"),
                         QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                         QStringLiteral("UnInhibit"));
        m << m_pwcookie;
        (void) verbose_dbus(m);
    }


    if(m_sscookie_valid) {
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("/ScreenSaver"),
                         QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("UnInhibit"));
        m << m_sscookie;
        (void) verbose_dbus(m);
    }

    m_screensaver_currently_enabled = NoYesUnknown::Yes;
}

void ScreenSaverManager::disable()
{
    if(m_screensaver_currently_enabled == NoYesUnknown::No) {
        return;
    }

    MYDBG("disabling screensaver, scheduling slot_screensaver_simulate_activity in %d sec", screensaver_simulate_activity_every_seconds);
    QTimer::singleShot(1000 * screensaver_simulate_activity_every_seconds, this, SLOT(slot_screensaver_simulate_activity()));

    xsetscreensaver_disable();

    {
        // service path interface method
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("/ScreenSaver"),
                         QStringLiteral("org.freedesktop.ScreenSaver"),
                         QStringLiteral("Inhibit")
                                                       );
        m << qApp->applicationName();
        m << QStringLiteral("because I told you so");
        QDBusMessage response = verbose_dbus(m);

        m_sscookie = 0;
        m_sscookie_valid = false;

        if(response.type() != QDBusMessage::ErrorMessage) {
            QList<QVariant> ret = response.arguments();

            if(ret.size() == 1) {
                QVariant r = ret[0];
                bool ok = false;
                unsigned u = r.toUInt(&ok);

                if(ok) {
                    m_sscookie = u;
                    m_sscookie_valid = true;
                    MYDBG("got ScreenSaver Inhibit cookie %u", m_sscookie);
                }
                else {
                    qWarning("DBUS ScreenSaver Inhibit: return value is not unsigned but a %s", r.typeName());
                }
            }
            else {
                qWarning("DBUS ScreenSaver Inhibit: not 1 return value but %d", int(ret.size()));
            }
        }
        else {
            MYDBG("could not do ScreenSaver Inhibit");
        }
    }

    {
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement"),
                         QStringLiteral("/org/freedesktop/PowerManagement"),
                         QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                         QStringLiteral("Inhibit"));
        m << qApp->applicationName();
        m << QStringLiteral("I said so");
        QDBusMessage response = verbose_dbus(m);

        m_pwcookie = 0;
        m_pwcookie_valid = false;

        if(response.type() != QDBusMessage::ErrorMessage) {
            QList<QVariant> ret = response.arguments();

            if(ret.size() == 1) {
                QVariant r = ret[0];
                bool ok = false;
                unsigned u = r.toUInt(&ok);

                if(ok) {
                    m_pwcookie = u;
                    m_pwcookie_valid = true;
                    MYDBG("got PowerManagement Inhibit cookie %u", m_pwcookie);
                }
                else {
                    qWarning("DBUS PowerManagement Inhibit: return value is not unsigned but a %s", r.typeName());
                }
            }
            else {
                qWarning("DBUS PowerManagement Inhibit: not 1 return value but %d", int(ret.size()));
            }
        }
        else {
            MYDBG("could not do PowerManagement Inhibit");
        }
    }

    xscreensaver_command_deactivate();

    m_screensaver_currently_enabled = NoYesUnknown::No;

}
