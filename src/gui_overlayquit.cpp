#include "gui_overlayquit.h"

#include <QPushButton>
#include <QTextEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

#include "safe_signals.h"

#define DEBUG_OQ

#ifdef DEBUG_ALL
#define DEBUG_OQ
#endif

#ifdef DEBUG_OQ
#define MYDBG(msg, ...) qDebug("OQ %s " msg, qPrintable(this->objectName()), ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif


void OverlayQuit::init(QWidget *in_body)
{
    m_body = in_body;
    m_body->hide();
    m_body->setParent(this);
    m_body->setFocusPolicy(Qt::StrongFocus);

    {
        QPushButton *q = new QPushButton(QLatin1String("QUIT"), this);
        XCONNECT(q, SIGNAL(clicked()), this, SLOT(slot_stop()), QUEUEDCONN);
        XCONNECT(q, SIGNAL(pressed()), this, SLOT(slot_stop()), QUEUEDCONN);
        q->setFlat(true);
        m_quit_button = q;
    }

    m_quit_button->setObjectName(objectName() + QLatin1String("_button"));
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
    m_quit_button->hide();
    m_quit_button->setAutoFillBackground(true);
    m_quit_button->setFocusPolicy(Qt::StrongFocus);

    this->setFocusPolicy(Qt::StrongFocus);

    setAutoFillBackground(true);
    hide();

    do_resize();
}


OverlayQuit::OverlayQuit(QWidget *parent, char const *const in_oname, QWidget *in_body) :
    QWidget(parent)
    , m_ori_focus(NULL)
{
    setObjectName(QLatin1String(in_oname));
    evfilter_body = false;
    init(in_body);
}

OverlayQuit::OverlayQuit(QWidget *parent, char const *const in_oname, const QString &text) :
    QWidget(parent)
    , m_ori_focus(NULL)
{
    setObjectName(QLatin1String(in_oname));
    evfilter_body = true;

    QTextEdit *te = new QTextEdit(text, this);
    te->setObjectName(objectName() + QLatin1String("_TE"));
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

// true - no further processing
// false - pass through
bool OverlayQuit::eventFilter(QObject *object, QEvent *event)
{
    if(!this->isVisible()) {
        MYDBG("eF: not visible, pass through");
        return false;
    }

    char const *oisdesc = NULL;

    if(object == m_body) {
        oisdesc = "object is body";
    }
    else if(object == this) {
        oisdesc = "object is this";
    }
    else if(object == m_quit_button) {
        oisdesc = "object is quit button";
    }
    else {
        MYDBG("eF: object is not a part of me, pass through");
        return false;
    }

    if(event->type() != QEvent::KeyPress) {
        if(event->type() != QEvent::MouseMove) {
            MYDBG("eF: %s, event is not a keypress, pass through", oisdesc);
        }

        return false;
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if(keyEvent->key() == Qt::Key_Escape) {
        MYDBG("eF: %s, ESC key detected", oisdesc);
        slot_stop();
        return true;
    }

    if(!evfilter_body) {
        MYDBG("eF: %s, not filtering events for body, pass through", oisdesc);
        return false;
    }

    if(keyEvent->key() == Qt::Key_Q) {
        MYDBG("eF: %s, filtering events for body, detected Q", oisdesc);
        slot_stop();
        return true;
    }

    if(keyEvent->key() == Qt::Key_Enter) {
        MYDBG("eF: %s, filtering events for body, detected Enter", oisdesc);
        slot_stop();
        return true;
    }

    if(keyEvent->key() == Qt::Key_Return) {
        MYDBG("eF: %s, filtering events for body, detected Return", oisdesc);
        slot_stop();
        return true;
    }

    MYDBG("eF: %s, filtering events for body, unknown key detected, pass through", oisdesc);
    // return QWidget::eventFilter(object, event)
    return false;
}

void OverlayQuit::focusOutEvent(QFocusEvent *event)
{
    if(this->isVisible()) {
        MYDBG("losing focus, but visible - regrabbing");
        this->setFocus(Qt::OtherFocusReason);
    }
    else {
        MYDBG("losing focus and hidden - its okay");
        QWidget::focusOutEvent(event);
    }
}

void OverlayQuit::focusInEvent(QFocusEvent *event)
{
    MYDBG("received focus");
    QWidget::focusInEvent(event);
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

void OverlayQuit::slot_of_destroyed(QObject *o)
{
    if(o == m_ori_focus) {
        MYDBG("deleting ori focus widget %s", qPrintable(o->objectName()));
        m_ori_focus = NULL;
    }
}

void OverlayQuit::set_ori_focus(QWidget *w)
{
    if(w == NULL) {
        return;
    }

    m_ori_focus = w;
    XCONNECT(m_ori_focus, SIGNAL(destroyed(QObject *)), this, SLOT(slot_of_destroyed(QObject *)));
}

void OverlayQuit::slot_stop()
{
    MYDBG("hide");
    hide();

    m_body->removeEventFilter(this);
    m_quit_button->removeEventFilter(this);
    this->removeEventFilter(this);

    if(m_ori_focus != NULL) {
        set_focus_raise(m_ori_focus);
    }

    emit sig_stopped();
}

void OverlayQuit::start_oq()
{
    MYDBG("show");

    m_body->installEventFilter(this);
    m_quit_button->installEventFilter(this);
    this->installEventFilter(this);

    do_resize();
    show();
    raise();
    m_body->raise();
    m_quit_button->raise();
    m_body->show();
    m_quit_button->show();

    if(evfilter_body) {
        m_quit_button->setFocus(Qt::OtherFocusReason);
    }
    else {
        m_body->setFocus(Qt::OtherFocusReason);
    }

}
