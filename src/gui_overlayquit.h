#ifndef GUI_OVERLAYQUIT_H
#define GUI_OVERLAYQUIT_H

#include <QWidget>
#include <QObject>

#include "focusstack.h"

QT_BEGIN_NAMESPACE
class QEvent;
QT_END_NAMESPACE

// OverlayQuit

class OverlayQuit: public QWidget
{
    Q_OBJECT

public:
    typedef QWidget super;

private:

    QWidget *m_quit_button;
    QWidget *m_body;
    FocusStack m_focusstack;
    bool evfilter_body;

private:
    // forbid
    OverlayQuit();
    OverlayQuit(const OverlayQuit &);
    OverlayQuit &operator=(const OverlayQuit &in);

public:

    explicit OverlayQuit(QWidget *parent, char const *const in_oname, QWidget *in_body);
    explicit OverlayQuit(QWidget *parent, char const *const in_oname, const QString &text);

protected:

    virtual bool event(QEvent *event);
    virtual bool eventFilter(QObject *object, QEvent *event);

signals:

    void sig_stopped();

public slots:

    // from base class and specific key presses
    void hide_and_return_focus();
    void show_and_take_focus(QWidget *oldfocusguess);

private:

    void init(QWidget *in_body);
    void do_resize();

};

#endif // GUI_OVERLAYQUIT_H
