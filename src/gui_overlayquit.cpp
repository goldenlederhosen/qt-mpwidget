#include "gui_overlayquit.h"

#include <QPushButton>
#include <QTextEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

#include "util.h"
#include "keepfocus.h"

#define DEBUG_OQ

#ifdef DEBUG_ALL
#define DEBUG_OQ
#endif

#ifdef DEBUG_OQ
#define MYDBG(msg, ...) qDebug("OQ " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif


void OverlayQuit::init(QWidget *in_body)
{
    body = in_body;
    body->hide();
    body->setParent(this);
    body->installEventFilter(this);
    body->setFocusPolicy(Qt::StrongFocus);

    {
        QPushButton *q = new QPushButton(QLatin1String("QUIT"), this);
        XCONNECT(q, SIGNAL(clicked()), this, SLOT(slot_stop()), QUEUEDCONN);
        XCONNECT(q, SIGNAL(pressed()), this, SLOT(slot_stop()), QUEUEDCONN);
        q->setFlat(true);
        quit_button = q;
    }

    quit_button->setObjectName(QLatin1String("OQ button"));
    {
        QPalette Pal(quit_button->palette());
        Pal.setColor(QPalette::Button, Qt::red);
        quit_button->setPalette(Pal);
    }
    {
        QFont f = quit_button->font();
        f.setPointSize(f.pointSize() * 2);
        quit_button->setFont(f);
    }
    quit_button->hide();
    quit_button->setAutoFillBackground(true);
    quit_button->setFocusPolicy(Qt::StrongFocus);
    quit_button->installEventFilter(this);

    this->setFocusPolicy(Qt::StrongFocus);
    this->installEventFilter(this);

    setAutoFillBackground(true);
    hide();

    do_resize();
}


OverlayQuit::OverlayQuit(QWidget *parent, QWidget *in_body) :
    QWidget(parent)
    , m_ori_focus(NULL)
{
    setObjectName(QLatin1String("OQ wbody"));
    evfilter_body = false;
    init(in_body);
}

OverlayQuit::OverlayQuit(QWidget *parent, const QString &text) :
    QWidget(parent)
    , m_ori_focus(NULL)
{
    setObjectName(QLatin1String("OQ wtext"));
    evfilter_body = true;

    QTextEdit *te = new QTextEdit(text, this);
    te->setObjectName(QLatin1String("OQ TE"));
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
// FIXME this is not triggering, instead key presses go to the UUI
bool OverlayQuit::eventFilter(QObject *object, QEvent *event)
{
    if(object != body && object != this && object != quit_button) {
        return false;
    }

    if(event->type() != QEvent::KeyPress) {
        return false;
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if(keyEvent->key() == Qt::Key_Escape) {
        slot_stop();
        return true;
    }

    if(evfilter_body) {
        if(keyEvent->key() == Qt::Key_Q) {
            slot_stop();
            return true;
        }

        if(keyEvent->key() == Qt::Key_Enter) {
            slot_stop();
            return true;
        }

        if(keyEvent->key() == Qt::Key_Return) {
            slot_stop();
            return true;
        }
    }

    return false;
}

void OverlayQuit::do_resize()
{
    QWidget const *mainw = QApplication::activeWindow();

    if(mainw == NULL) {
        mainw = this->parentWidget();

        if(mainw == NULL) {
            MYDBG("%s: found neither main window nor parent widget", qPrintable(this->objectName()));
        }
        else {
            MYDBG("%s: found parent widget, %dx%d", qPrintable(this->objectName()), mainw->width(), mainw->height());
        }
    }
    else {
        MYDBG("%s: found main window, %dx%d", qPrintable(this->objectName()), mainw->width(), mainw->height());
    }

    const int  mw = (mainw == NULL ? 100 : mainw->width());
    const int  mh = (mainw == NULL ? 100 : mainw->height());
    this->resize(mw, mh);
    this->move(0, 0);

    const int qh = quit_button->sizeHint().height();

    quit_button->resize(mw, qh);
    quit_button->move(0, 0);
    body->resize(mw, mh - qh);
    body->move(0, qh);
}

void OverlayQuit::slot_of_destroyed(QObject *o)
{
    if(o == m_ori_focus) {
        MYDBG("%s: deleting ori focus widget %s", qPrintable(this->objectName()), qPrintable(o->objectName()));
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
    MYDBG("%s: hide", qPrintable(this->objectName()));
    hide();

    if(m_ori_focus != NULL) {
        focus_should_go_to(m_ori_focus);
    }

    emit sig_stopped();
}

void OverlayQuit::start_oq()
{
    MYDBG("%s: show", qPrintable(this->objectName()));
    do_resize();
    show();
    raise();
    body->raise();
    quit_button->raise();
    body->show();
    quit_button->show();

    if(evfilter_body) {
        focus_should_go_to(quit_button);
    }
    else {
        focus_should_go_to(body);
    }

}
