#ifndef SCREENSAVERMANAGER_H
#define SCREENSAVERMANAGER_H

#include <QObject>

#include "util.h"

typedef bool (*predicate_t)(void *);

class ScreenSaverManager: public QObject
{
    Q_OBJECT
public:
    typedef QObject super;
private:
    NoYesUnknown m_screensaver_currently_enabled;
    unsigned m_sscookie, m_pwcookie;
    bool m_sscookie_valid, m_pwcookie_valid;
    predicate_t m_screensaver_should_be_active;
    void *m_screensaver_should_be_active_payload;
    NoYesUnknown m_found_own_xscreensaver;
private:
    void xscreensaver_command_deactivate();
private:
    // forbid
    ScreenSaverManager();
    ScreenSaverManager(const ScreenSaverManager &);
    ScreenSaverManager &operator=(const ScreenSaverManager &in);
public:
    ScreenSaverManager(QObject *parent, predicate_t func, void *payload);
    virtual ~ScreenSaverManager();
    void enable();
    void disable();
public slots:
    void slot_screensaver_simulate_activity();
private:
    bool screensaver_should_be_active() const;
protected:
    virtual bool event(QEvent *event);

};

#endif // SCREENSAVERMANAGER_H
