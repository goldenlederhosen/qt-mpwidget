#ifndef SCREENSAVERMANAGER_H
#define SCREENSAVERMANAGER_H

#include <QObject>

#include "util.h"

typedef bool (*predicate_t)(void *);

class ScreenSaverManager: QObject {
    Q_OBJECT
private:
    NoYesUnknown m_screensaver_currently_enabled;
    unsigned m_sscookie, m_pwcookie;
    bool m_sscookie_valid, m_pwcookie_valid;
    predicate_t m_screensaver_should_be_active;
    void *m_screensaver_should_be_active_payload;
    // forbid
    ScreenSaverManager();
public:
    ScreenSaverManager(QObject *parent, predicate_t func, void *payload);
    virtual ~ScreenSaverManager();
    void enable();
    void disable();
public slots:
    void slot_screensaver_simulate_activity();
private:
    bool screensaver_should_be_active() const;
};

#endif // SCREENSAVERMANAGER_H
