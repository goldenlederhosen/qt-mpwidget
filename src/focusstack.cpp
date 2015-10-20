#include "focusstack.h"

#include "util.h"
#include <QApplication>

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "FS"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

void FocusStack::set_ori_focus(QWidget *w)
{
    if(w == m_ori_focus) {
        MYDBG("%s set_ori_focus: keeping ori_focus %s", qPrintable(object_2_name(m_targetwidget)), qPrintable(object_2_name(m_ori_focus)));
        return;
    }

    if(m_ori_focus != NULL) {
        MYDBG("%s set_ori_focus: disconnecting old ori_focus %s", qPrintable(object_2_name(m_targetwidget)), qPrintable(object_2_name(m_ori_focus)));
        m_ori_focus = NULL;
    }

    if(w == NULL) {
        MYDBG("%s set_ori_focus: given NULL widget??", qPrintable(object_2_name(m_targetwidget)));
        return;
    }

    MYDBG("%s set_ori_focus: setting %s", qPrintable(object_2_name(m_targetwidget)), qPrintable(object_2_name(w)));

    m_ori_focus = w;
}

void FocusStack::show_and_take_focus(QWidget *newfocus, QWidget *oldfocusguess)
{
    MYDBG("%s show_and_take_focus", qPrintable(object_2_name(m_targetwidget)));

    if(newfocus == NULL) {
        newfocus = m_targetwidget;
    }

    const bool newvisib = newfocus->isVisible();

    QWidget *oldfocus = QApplication::focusWidget();

    if(oldfocus == NULL) {
        if(oldfocusguess == NULL) {
            qFatal("%s is trying to make %s (%s) the new front focus widget. Current focus: outside window. Thats bad because I will not know whom to give focus back to"
                   , qPrintable(object_2_name(m_targetwidget))
                   , qPrintable(object_2_name(newfocus))
                   , newvisib ? "currently visible" : "currently hidden"
                  );
        }
        else {
            oldfocus = oldfocusguess;
            qWarning("%s can not guess on current focus: focus is outside window. Will use hint %s"
                     , qPrintable(object_2_name(m_targetwidget))
                     , qPrintable(object_2_name(oldfocusguess))
                    );
        }
    }
    else {
        if(oldfocusguess != NULL && oldfocusguess != oldfocus) {
            qWarning("%s: current focus is %s, but hint is %s. Will use current focus for the stack"
                     , qPrintable(object_2_name(m_targetwidget))
                     , qPrintable(object_2_name(oldfocus))
                     , qPrintable(object_2_name(oldfocusguess))
                    );
        }
    }

    if(oldfocus == newfocus) {
        qFatal("oldfocus %s already == newfocus %s"
               , qPrintable(object_2_name(oldfocus))
               , qPrintable(object_2_name(newfocus))
              );
    }

    const bool oldvisib = oldfocus->isVisible();

    MYDBG("%s is trying to make %s (%s) the new front focus widget. Current focus: %s (%s)"
          , qPrintable(object_2_name(m_targetwidget))
          , qPrintable(object_2_name(newfocus))
          , newvisib ? "currently visible" : "currently hidden"
          , qPrintable(object_2_name(oldfocus))
          , oldvisib ? "currently visible" : "currently hidden"
         );

    if(!oldvisib) {
        qFatal("old focus is not visible");
    }

    m_targetwidget->show();

    newfocus->raise();
    newfocus->setFocus(Qt::OtherFocusReason);

    MYDBG("%s show_and_take_focus: found focusWidget %s, setting ori_focus to it"
          , qPrintable(object_2_name(m_targetwidget))
          , qPrintable(object_2_name(oldfocus))
         );
    set_ori_focus(oldfocus);
}

void FocusStack::hide_and_return_focus()
{
    MYDBG("%s hide_and_return_focus", qPrintable(object_2_name(m_targetwidget)));

    if(m_ori_focus == NULL) {
        MYDBG("%s hide_and_return_focus: no original focus set yet, only calling hide()", qPrintable(object_2_name(m_targetwidget)));
        m_targetwidget->hide();
        return;
    }

    MYDBG("%s hide_and_return_focus: giving focus back to original %s", qPrintable(object_2_name(m_targetwidget)), qPrintable(object_2_name(m_ori_focus)));

    QWidget *const curfocus = QApplication::focusWidget();

    if(curfocus == m_ori_focus) {
        qFatal("curfocus %s == m_ori_focus %s? Expected it to be %s"
               , qPrintable(object_2_name(curfocus))
               , qPrintable(object_2_name(m_ori_focus))
               , qPrintable(object_2_name(m_targetwidget))
              );
    }

    if(curfocus && curfocus != m_targetwidget) {
        qFatal("curfocus %s != targetwidget %s?"
               , qPrintable(object_2_name(curfocus))
               , qPrintable(object_2_name(m_targetwidget))
              );
    }

    const bool newvisib = m_ori_focus->isVisible();

    if(curfocus == NULL) {
        MYDBG("%s is trying to return focus to %s (%s). Current focus: outside window. We'll let it slide..."
              , qPrintable(object_2_name(m_targetwidget))
              , qPrintable(object_2_name(m_ori_focus))
              , newvisib ? "currently visible" : "currently hidden"
             );
    }
    else {
        const bool oldvisib = curfocus->isVisible();
        MYDBG("%s is trying to return focus to %s (%s). Current focus: %s (%s)"
              , qPrintable(object_2_name(m_targetwidget))
              , qPrintable(object_2_name(m_ori_focus))
              , newvisib ? "currently visible" : "currently hidden"
              , qPrintable(object_2_name(curfocus))
              , oldvisib ? "currently visible" : "currently hidden"
             );

        if(!oldvisib) {
            qFatal("old focus is not visible");
        }
    }

    m_targetwidget->hide();

    m_ori_focus->show();
    m_ori_focus->raise();
    m_ori_focus->setFocus(Qt::OtherFocusReason);

}


