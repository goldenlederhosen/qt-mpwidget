#include "keepfocus.h"
#include "util.h"

#include "focusmanager.h"

#include <QApplication>
#include <QWidget>
#include <QObject>

#define DEBUG_FOCUS

#ifdef DEBUG_ALL
#define DEBUG_FOCUS
#endif

#ifdef DEBUG_FOCUS
#define MYDBG(msg, ...) qDebug("FOCUS " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

FocusManager *fmanager = NULL;

void init_fmanager()
{
    if(fmanager != NULL) {
        return;
    }

    fmanager = new FocusManager();
    XCONNECT(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)), fmanager, SLOT(slot_focuschanged(QWidget *, QWidget *)));
}

static QWidget *focuswidget = NULL;

bool focus_is_outside()
{
    init_fmanager();
    QWidget *w = QApplication::focusWidget();

    if(w == NULL) {
        MYDBG("Focus is outside of app");
        return true;
    }
    else {
        MYDBG("Focus is inside of app with %s", qPrintable(w->objectName()));
        return false;
    }
}

#define prevname ( prevfw == NULL ? QLatin1String("unknown") : prevfw->objectName())
#define newname ( newfw == NULL ? QLatin1String("nobody") : newfw->objectName())
#define curname (QApplication::focusWidget() == NULL ? QLatin1String("outside") : QApplication::focusWidget()->objectName())

void focus_should_go_to(QWidget *newfw, char const *const file, int line)
{
    init_fmanager();

    QWidget *prevfw = focuswidget;

    MYDBG("%s:%d: focus_should_go_to(%s). prev=%s, cur=%s", file, line, qPrintable(newname), qPrintable(prevname), qPrintable(curname));

    //if(newfw == focuswidget) {
    //    MYDBG("   multiple calls to focus_should_go_to(%s)", qPrintable(newname));
    //    return;
    // }

    focuswidget = newfw;
    fmanager->register_destroy_watcher(focuswidget);

    if(focuswidget != NULL) {
        QWidget *w = QApplication::focusWidget();;

        if(w != NULL) {
            MYDBG("   giving focus to %s from %s",  qPrintable(newname), qPrintable(curname));
            focuswidget->show();
            focuswidget->raise();
            focuswidget->setFocus();
        }
        else {
            MYDBG("   not manually setting since focus is now outside");
        }
    }
    else {
        MYDBG("   not setting manual focus: setting to nobody");
    }

    //return prevfw;
}

void focus_should_definitely_not_be(QObject *bad, char const *const file, int line)
{
    if(bad == NULL) {
        return;
    }

    if(focuswidget == bad) {
        MYDBG("%s:%d: focus_should_definitely_not_be(%s)", file, line, qPrintable(focuswidget->objectName()));
        focuswidget = NULL;
        return;
    }

    MYDBG("%s:%d: focus_should_definitely_not_be(%s), but we dont care about that", file, line, qPrintable(bad->objectName()));

}

QWidget *focus_should_currently_be_at()
{
    init_fmanager();
    return focuswidget;
}

