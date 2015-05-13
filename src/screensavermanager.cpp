#include "screensavermanager.h"
#include "dbus.h"
#include "xsetscreensaver.h"

#include <QTimer>
#include <QDBusMessage>
#include <QApplication>

#define DEBUG_SSMN

#ifdef DEBUG_ALL
#define DEBUG_SSMN
#endif

#ifdef DEBUG_SSMN
#define MYDBG(msg, ...) qDebug("SSMN " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

// when we have the screensaver disabled, simulate activity every N seconds (the Inhibit interface does not always work)
static const int screensaver_simulate_activity_every_seconds = 30;

// run with QDBUS_DEBUG=1 for more info. Unfortunately that can not be switched on at runtime

ScreenSaverManager::ScreenSaverManager(QObject *parent, predicate_t func, void *payload):
    QObject(parent),
    m_screensaver_currently_enabled(NoYesUnknown::Unknown),
    m_sscookie(0),
    m_pwcookie(0),
    m_sscookie_valid(false),
    m_pwcookie_valid(false),
    m_screensaver_should_be_active(func),
    m_screensaver_should_be_active_payload(payload)
{
}

ScreenSaverManager::~ScreenSaverManager()
{
    enable();
}

bool ScreenSaverManager::screensaver_should_be_active() const
{
    return (*m_screensaver_should_be_active)(m_screensaver_should_be_active_payload);
}

void ScreenSaverManager::slot_screensaver_simulate_activity()
{
    if(screensaver_should_be_active()) {
        MYDBG("screensaver_simulate_activity: screensaver should be active, exiting");
        return;
    }

    MYDBG("screensaver_simulate_activity: screensaver should be inhibited, simulating and re-raising");

    {
        QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("/ScreenSaver"),
                         QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("SimulateUserActivity"));

        (void) verbose_dbus(m);
    }

    QTimer::singleShot(1000 * screensaver_simulate_activity_every_seconds, this, SLOT(slot_screensaver_simulate_activity()));
}


void ScreenSaverManager::enable()
{
    if(m_screensaver_currently_enabled == NoYesUnknown::Yes) {
        return;
    }

    MYDBG("enabling screensaver");

    xsetscreensaver_enable();

    if(m_pwcookie_valid) {
        QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.PowerManagement"),
                         QLatin1String("/org/freedesktop/PowerManagement"),
                         QLatin1String("org.freedesktop.PowerManagement.Inhibit"),
                         QLatin1String("UnInhibit"));
        m << m_pwcookie;
        (void) verbose_dbus(m);
    }


    if(m_sscookie_valid) {
        QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("/ScreenSaver"),
                         QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("UnInhibit"));
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

    MYDBG("disabling screensaver, setting simulateactivity timer");
    QTimer::singleShot(1000 * screensaver_simulate_activity_every_seconds, this, SLOT(slot_screensaver_simulate_activity()));

    xsetscreensaver_disable();

    {
        // service path interface method
        QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("/ScreenSaver"),
                         QLatin1String("org.freedesktop.ScreenSaver"),
                         QLatin1String("Inhibit")
                                                       );
        m << qApp->applicationName();
        m << QLatin1String("because I told you so");
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
        QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.PowerManagement"),
                         QLatin1String("/org/freedesktop/PowerManagement"),
                         QLatin1String("org.freedesktop.PowerManagement.Inhibit"),
                         QLatin1String("Inhibit"));
        m << qApp->applicationName();
        m << QLatin1String("I said so");
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


    m_screensaver_currently_enabled = NoYesUnknown::No;

}
