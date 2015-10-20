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

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

public:
    typedef QMainWindow super;
private:

    QStringList mfns;
    bool fullscreen;
    MpWidget *MP;
    CropDetector *cd;
    QString currently_playing_mfn;
    QStringList falangs;
    QStringList palangs;
    QStringList pslangs;
    bool might_get_remote_files;
private:
    // forbid
    PlayerWindow();
    PlayerWindow(const PlayerWindow &);
    PlayerWindow &operator=(const PlayerWindow &in);
public:
    PlayerWindow(bool in_fullscreen, const QStringList &in_mfns, const QStringList &in_falangs, const QStringList &in_palangs, const QStringList &in_pslangs);
    virtual ~PlayerWindow();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual bool event(QEvent *event);

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
