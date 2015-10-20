#ifndef FOCUSSTACK_H
#define FOCUSSTACK_H

#include <QPointer>
#include <QWidget>
#include <QString>

class FocusStack
{
private:
    QPointer<QWidget> m_ori_focus;
    QPointer<QWidget> m_targetwidget;

    void set_ori_focus(QWidget *w);
    FocusStack();
public:
    FocusStack(QWidget *in_targetwidget)
        : m_targetwidget(in_targetwidget)
    {}
    void show_and_take_focus(QWidget *newfocus, QWidget *oldfocusguess);
    void hide_and_return_focus();
};

#endif // FOCUSSTACK_H
