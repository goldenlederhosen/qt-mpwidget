#ifndef GUI_OVERLAYQUIT_H
#define GUI_OVERLAYQUIT_H

#include <QWidget>
#include <QObject>

QT_BEGIN_NAMESPACE
class QEvent;
QT_END_NAMESPACE

// OverlayQuit

class OverlayQuit: public QWidget {
    Q_OBJECT

private:

    QWidget *m_quit_button;
    QWidget *m_body;
    QWidget *m_ori_focus;
    bool evfilter_body;

public:

    explicit OverlayQuit(QWidget *parent, char const *const in_oname, QWidget *in_body);
    explicit OverlayQuit(QWidget *parent, char const *const in_oname, const QString &text);
    void start_oq();
    void set_ori_focus(QWidget *w);

protected:

    virtual bool eventFilter(QObject *object, QEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void focusInEvent(QFocusEvent *event);

signals:

    void sig_stopped();

public slots:

    // from base class and specific key presses
    void slot_stop();
    void slot_of_destroyed(QObject *o);

private:

    void init(QWidget *in_body);
    void do_resize();

};

#endif // GUI_OVERLAYQUIT_H
