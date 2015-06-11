#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QMainWindow>
#include "util.h"
#include "mpprocess.h"

QT_BEGIN_NAMESPACE
class QString;
class QProcess;
class QLabel;
class QUrl;
class QPushButton;
QT_END_NAMESPACE

class MpWidget;
class CropDetector;

class PlayerWindow : public QMainWindow {
    Q_OBJECT

private:

    QStringList mfns;
    bool fullscreen;
    MpWidget *MP;
    CropDetector *cd;
    QString currently_playing_mfn;

public:
    PlayerWindow(bool in_fullscreen, const QStringList &in_mfns);
    ~PlayerWindow();

protected:
    virtual void resizeEvent(QResizeEvent *event);

private slots:

    // from the MpWidget
    void slot_MP_state_changed(MpState, MpState);
    void slot_MP_error_while(QString reason, QString url, MpMediaInfo mmi, double lastpos);
    void slot_MP_does_not_work();
    void slot_MP_died(QObject *dead);
    // from a delayed execution
    void slot_MP_start();
    // from the cropdetector
    void slot_cdDetected(bool, QString, QString);

private:

    void activate_this_window();
    void resizemyself();
    void init_MP_vars();
    void MP_finished(bool success, const QString &errstr = QString());
    void MP_window_correct();
    void init_MP_object();

};

#endif
