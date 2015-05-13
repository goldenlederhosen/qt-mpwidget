#ifndef FOCUSMANAGER_H
#define FOCUSMANAGER_H

#include <QObject>
class QWidget;

class FocusManager: public QObject {
    Q_OBJECT
public:
    FocusManager(QObject *parent = 0): QObject(parent) {}
public slots:
    void slot_focuschanged(QWidget *old, QWidget *now);
    void slot_destroyed(QObject *w);
public:
    void register_destroy_watcher(QWidget *w);
};

#endif // FOCUSMANAGER_H
