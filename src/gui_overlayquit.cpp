#include "gui_overlayquit.h"

#include <QPushButton>
#include <QTextEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

#include "safe_signals.h"
#include "event_desc.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "OQ"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, "%s " msg, qPrintable(this->objectName()), ##__VA_ARGS__)

void OverlayQuit::init(QWidget *in_body)
{
    m_body = in_body;
    m_body->setParent(this);
    m_body->setFocusPolicy(Qt::NoFocus);

    {
        QPushButton *q = new QPushButton(QStringLiteral("QUIT"));
        q->setObjectName(objectName() + QStringLiteral("_qbutton"));
        q->setParent(this); // do this after the objectname, so the signal knows who this is
        XCONNECT(q, SIGNAL(clicked()), this, SLOT(hide_and_return_focus()), QUEUEDCONN);
        XCONNECT(q, SIGNAL(pressed()), this, SLOT(hide_and_return_focus()), QUEUEDCONN);
        q->setFlat(true);
        m_quit_button = q;
    }

    {
        QPalette Pal(m_quit_button->palette());
        Pal.setColor(QPalette::Button, Qt::red);
        m_quit_button->setPalette(Pal);
    }
    {
        QFont f = m_quit_button->font();
        f.setPointSize(f.pointSize() * 2);
        m_quit_button->setFont(f);
    }
    m_quit_button->setAutoFillBackground(true);

    m_quit_button->setFocusPolicy(Qt::NoFocus);

    this->setFocusPolicy(Qt::NoFocus);

    setAutoFillBackground(true);
    hide();

    do_resize();
}


OverlayQuit::OverlayQuit(QWidget *parent, char const *const in_oname_latin1lit, QWidget *in_body) :
    super()
    , m_focusstack(this)
{
    setObjectName(QLatin1String(in_oname_latin1lit));
    setParent(parent); // do this after the objectname, so the signal knows who this is
    evfilter_body = false;
    MYDBG("creating with body %s", qPrintable(object_2_name(in_body)));
    init(in_body);
}

OverlayQuit::OverlayQuit(QWidget *parent, char const *const in_oname_latin1lit, const QString &text) :
    super()
    , m_focusstack(this)
{
    setObjectName(QLatin1String(in_oname_latin1lit));
    setParent(parent); // do this after the objectname, so the signal knows who this is
    evfilter_body = true;

    MYDBG("creating with text \"%s...\"", qPrintable(text.left(20)));

    QTextEdit *te = new QTextEdit(text);
    te->setObjectName(objectName() + QStringLiteral("_TE"));
    te->setParent(this); // do this after the objectname, so the signal knows who this is
    te->setReadOnly(true);
    te->setTextInteractionFlags(Qt::NoTextInteraction);
    te->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    te->setFrameShadow(QFrame::Plain);
    te->setFrameShape(QFrame::NoFrame);
    QFont f = te->font();
    f.setPointSize(f.pointSize() * 2);
    te->setFont(f);

    init(te);
}

bool OverlayQuit::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}

// true - no further processing
// false - pass through
bool OverlayQuit::eventFilter(QObject *object, QEvent *event)
{
    if(!this->isVisible()) {
        MYDBG("eventFilter: not visible, pass through");
        return false;
    }

    log_qeventFilter(category(), object, event);

    char const *oisdesc = NULL;

    if(object == m_body) {
        if(evfilter_body) {
            oisdesc = "object is body and we handle its Q/Esc/... events";
        }
        else {
            oisdesc = "object is body and handling its own events";
        }
    }
    else if(object == this) {
        oisdesc = "object is this";
    }
    else if(object == m_quit_button) {
        oisdesc = "object is quit button";
    }
    else {
        MYDBG("eventFilter: object %s is not a part of me, pass through. Thats kind of weird...", qPrintable(object_2_name(object)));
        return false;
    }

    if(event->type() != QEvent::KeyPress) {
        // only print msg if it is not a mousemove to avoid spamming
        if(event->type() != QEvent::MouseMove) {
            MYDBG("eventFilter: %s, event is not a keypress, pass through", oisdesc);
        }

        return false;
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    // object is body, this or qbutton, event is keyboard

    if(keyEvent->key() == Qt::Key_Escape) {
        MYDBG("eventFilter: %s, ESC key detected. calling hide_and_return_focus", oisdesc);
        hide_and_return_focus();
        return true;
    }

    if(object == m_body && !evfilter_body) {
        MYDBG("eventFilter: %s, not filtering events for body, pass through", oisdesc);
        return false;
    }

    // the object is not the body; or it is the body and that body is a TextEdit

    if(keyEvent->key() == Qt::Key_Q) {
        MYDBG("eventFilter: %s, filtering events for body, detected Q. calling hide_and_return_focus", oisdesc);
        hide_and_return_focus();
        return true;
    }

    if(keyEvent->key() == Qt::Key_Enter) {
        MYDBG("eventFilter: %s, filtering events for body, detected Enter. calling hide_and_return_focus", oisdesc);
        hide_and_return_focus();
        return true;
    }

    if(keyEvent->key() == Qt::Key_Return) {
        MYDBG("eventFilter: %s, filtering events for body, detected Return. calling hide_and_return_focus", oisdesc);
        hide_and_return_focus();
        return true;
    }

    MYDBG("eventFilter: %s, filtering events for body, unknown key detected, pass through", oisdesc);
    // return super::eventFilter(object, event)
    return false;
}

void OverlayQuit::do_resize()
{
    QWidget const *mainw = QApplication::activeWindow();

    if(mainw == NULL) {
        mainw = this->parentWidget();

        if(mainw == NULL) {
            MYDBG("found neither main window nor parent widget");
        }
        else {
            MYDBG("found parent widget, %dx%d", mainw->width(), mainw->height());
        }
    }
    else {
        MYDBG("found main window, %dx%d", mainw->width(), mainw->height());
    }

    const int  mw = (mainw == NULL ? 100 : mainw->width());
    const int  mh = (mainw == NULL ? 100 : mainw->height());
    this->resize(mw, mh);
    this->move(0, 0);

    const int qh = m_quit_button->sizeHint().height();

    m_quit_button->resize(mw, qh);
    m_quit_button->move(0, 0);
    m_body->resize(mw, mh - qh);
    m_body->move(0, qh);
}

void OverlayQuit::hide_and_return_focus()
{
    MYDBG("hide_and_return_focus: hide etc");

    m_focusstack.hide_and_return_focus();

    m_body->removeEventFilter(this);
    m_quit_button->removeEventFilter(this);
    this->removeEventFilter(this);

    MYDBG("EMIT sig_stopped");
    emit sig_stopped();
}

void OverlayQuit::show_and_take_focus(QWidget *oldfocusguess)
{
    MYDBG("show_and_take_focus: install eventFilters, show, raise etc");

    QWidget *newfocus;

    if(evfilter_body) {
        MYDBG("giving focus to quit button");
        newfocus = this;
    }
    else {
        MYDBG("giving focus to body");
        newfocus = m_body;
    }

    m_focusstack.show_and_take_focus(newfocus, oldfocusguess);

    m_body->installEventFilter(this);
    m_quit_button->installEventFilter(this);
    this->installEventFilter(this);

    do_resize();

}
