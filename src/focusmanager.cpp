#include "focusmanager.h"
#include "keepfocus.h"
#include "util.h"

#include <QApplication>
#include <QWidget>

#define DEBUG_FOCUSMNGR

#ifdef DEBUG_ALL
#define DEBUG_FOCUSMNGR
#endif

#ifdef DEBUG_FOCUSMNGR
#define MYDBG(msg, ...) qDebug("FOCUSM " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

void FocusManager::slot_focuschanged(QWidget *old, QWidget *now)
{
    QWidget *w = QApplication::focusWidget();
    QWidget *should = focus_should_currently_be_at();

#ifdef DEBUG_FOCUSMNGR
    QString ostr;

    if(old != NULL) {
        ostr = old->objectName();

        if(ostr.isEmpty()) {
            ostr = QLatin1String("old/unnamed");
        }
    }

    QString nstr;

    if(now != NULL) {
        nstr = now->objectName();

        if(nstr.isEmpty()) {
            nstr = QLatin1String("now/unnamed");
        }
    }



    QString wstr;

    if(w == NULL) {
        wstr = QLatin1String("outside");
    }
    else {
        wstr = w->objectName();

        if(wstr.isEmpty()) {
            wstr = QLatin1String("focus/unnamed");
        }
    }

    QString shouldstring;

    if(should == NULL) {
        shouldstring = QLatin1String("unknown");
    }
    else {
        shouldstring = should->objectName();

        if(shouldstring.isEmpty()) {
            shouldstring = QLatin1String("unnamed");
        }
    }

    QString msg = QLatin1String("focus changed");

    if(!ostr.isEmpty()) {
        msg += QLatin1String(" from ");
        msg += ostr;
    }

    if(!nstr.isEmpty()) {
        msg += QLatin1String(" to ");
        msg += nstr;
    }

    msg += QLatin1String(", now ");
    msg += wstr;
    msg += QLatin1String(", should ");
    msg += shouldstring;

    MYDBG("%s", qPrintable(msg));

#endif

    if(w != NULL && should != NULL && w != should) {
        MYDBG("setting focus to %s", qPrintable(should->objectName()));
        should->raise();
        should->setFocus();
    }

    return;
}

void FocusManager::register_destroy_watcher(QWidget *w)
{
    XCONNECT(w, SIGNAL(destroyed(QObject *)), this, SLOT(slot_destroyed(QObject *)));
}

void FocusManager::slot_destroyed(QObject *w)
{
    if(w == NULL) {
        return;
    }

    MYDBG("%s is being destroyed", qPrintable(w->objectName()));
    focus_should_definitely_not_be(w);
}
