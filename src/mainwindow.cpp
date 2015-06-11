#include "mainwindow.h"

#include <QFile>
#include <QLatin1String>
#include <QRegExp>
#include <QProcess>
#include <QUrl>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>

#include "mpwidget.h"
#include "cropdetector.h"
#include "gui_overlayquit.h"

#define DEBUG_IV

#ifdef DEBUG_ALL
#define DEBUG_IV
#endif

#ifdef DEBUG_IV
#define MYDBG(msg, ...) qDebug("IV " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

// most of episode watched if we stopped
// not earlier than so many seconds before the end
const double MIN_LASTPOS_DIFF_ABS_SECONDS = 300.;
// or relatively speaking, this many percent
// of the total length before the end
// 4.5% = 4 minutes on a 90 minutes movie
const double MIN_LASTPOS_DIFF_REL_PERCENT = 4.5;

void PlayerWindow::slot_MP_died(QObject *dead)
{
    if(dead == (QObject *)MP) {
        MP = NULL;
    }
}

void PlayerWindow::init_MP_object()
{

    if(MP != NULL) {
        MP->deleteLater();
        MP = NULL;
    }

    MP = new MpWidget(this, fullscreen);
    XCONNECT(MP, SIGNAL(destroyed(QObject *)), this, SLOT(slot_MP_died(QObject *)));

    QStringList MP_args_default;
    MP_args_default += QLatin1String("-osdlevel");
    MP_args_default += QLatin1String("1");
    MP_args_default += QLatin1String("-ass");
    MP_args_default += QLatin1String("-font");
    MP_args_default += QLatin1String("Verdana");
    MP_args_default += QLatin1String("-fontconfig");
    MP_args_default += QLatin1String("-volume");
    MP_args_default += QLatin1String("100");
    MP_args_default += QLatin1String("-softvol");
    MP_args_default += QLatin1String("-subcp");
    MP_args_default += QLatin1String("UTF-8");
    MP_args_default += QLatin1String("-channels");
    MP_args_default += QLatin1String("2");
    MP_args_default += QLatin1String("-framedrop");

    MP_args_default += QLatin1String("-cache");
    MP_args_default += QLatin1String("16384"); // kBytes
    MP_args_default += QLatin1String("-cache-min");
    MP_args_default += QLatin1String("5");

    MP_args_default += QLatin1String("-mc");
    MP_args_default += QLatin1String("3");

    QStringList MP_args = MP_args_default;

    const QByteArray MP_OPTS_APPEND = qgetenv("MP_OPTS_APPEND");
    const QByteArray MP_OPTS_OVERRI = qgetenv("MP_OPTS_OVERRIDE");

    if(!MP_OPTS_OVERRI.isEmpty()) {
        MP_args = doSplitArgs(QString::fromLocal8Bit(MP_OPTS_APPEND.constData()));
    }

    if(!MP_OPTS_APPEND.isEmpty()) {
        QStringList l = doSplitArgs(QString::fromLocal8Bit(MP_OPTS_APPEND.constData()));
        MP_args += l;
    }

    MP->setMplayerArgs(MP_args);

    MP->setVideoOutput(get_MP_VO());

    const QString gs = QLatin1String("ger");
    const QString es = QLatin1String("eng");

    {
        QStringList preferred_alangs;

        const QByteArray PALANGS = qgetenv("MP_PALANGS");

        if(!PALANGS.isEmpty()) {
            QString sPALANGS = QString::fromLocal8Bit(PALANGS.constData());
            preferred_alangs = sPALANGS.split(QLatin1Char(','), QString::SkipEmptyParts);
            MYDBG("setting preferred alangs %s", qPrintable(preferred_alangs.join(QLatin1String(", "))));
        }

        preferred_alangs.append(gs);
        preferred_alangs.append(es);

        preferred_alangs.removeDuplicates();
        MP->set_preferred_alangs(preferred_alangs);

    }

    {
        QStringList preferred_slangs;

        const QByteArray PSLANGS = qgetenv("MP_PSLANGS");

        if(!PSLANGS.isEmpty()) {
            QString sPSLANGS = QString::fromLocal8Bit(PSLANGS.constData());
            preferred_slangs = sPSLANGS.split(QLatin1Char(','), QString::SkipEmptyParts);
            MYDBG("setting preferred slangs %s", qPrintable(preferred_slangs.join(QLatin1String(", "))));
        }

        preferred_slangs.append(es);
        preferred_slangs.append(gs);

        preferred_slangs.removeDuplicates();

        MP->set_preferred_slangs(preferred_slangs);
    }

    MP->start_process();
    MP->resize(this->size());
    MP->move(0, 0);
    MP->lower();
    MP->hide();
    XCONNECT(MP, SIGNAL(sig_stateChanged(MpState, MpState)), this, SLOT(slot_MP_state_changed(MpState, MpState)), QUEUEDCONN);
    XCONNECT(MP, SIGNAL(sig_error_while(QString, QString, MpMediaInfo, double)), this, SLOT(slot_MP_error_while(QString, QString, MpMediaInfo, double)), QUEUEDCONN);
    XCONNECT(MP, SIGNAL(sig_I_give_up()), this, SLOT(slot_MP_does_not_work()), QUEUEDCONN);
}

PlayerWindow::PlayerWindow(bool in_fullscreen, const QStringList &in_mfns): QMainWindow()
{
    setObjectName(QLatin1String("PlayerWindow"));
    cd = NULL;
    MP = NULL;

    fullscreen = in_fullscreen;
    mfns = in_mfns;

    QDesktopWidget *mydesk = QApplication::desktop();

    if(fullscreen) {
        MYDBG("going fullscreen");
        this->showFullScreen();
        const QRect sg = mydesk->screenGeometry(this);
        const int w = sg.width();
        const int h = sg.height();
        MYDBG("resizing to %dx%d", w, h);
        resize(w, h);

    }
    else {
        const QRect sg = mydesk->availableGeometry(this);
        const int w = qMin(960, sg.width() - 20);
        const int h = qMin(540, sg.height() - 20);
        MYDBG("resizing to %dx%d", w, h);
        resize(w, h);
    }

    MP = NULL;
    init_MP_object();
    setCentralWidget(MP);
    set_focus_raise(MP);
    QTimer::singleShot(0, this, SLOT(slot_MP_start()));

}

PlayerWindow::~PlayerWindow()
{
    if(MP != NULL) {
        delete MP;
    }
}

void PlayerWindow::resizemyself()
{
    if(MP && MP->isVisible()) {
        MP->resize(this->size());
        MP->move(0, 0);
    }
}

void PlayerWindow::resizeEvent(QResizeEvent * /* event */)
{
    MYDBG("ImageViewer resizeEvent");
    resizemyself();

}

void PlayerWindow::activate_this_window()
{
    raise();
    activateWindow();
}

void PlayerWindow::MP_finished(bool success, const QString &errstr)
{
    if(success) {
        MYDBG("ImageViewer viewer has finished successfully");
    }
    else {
        MYDBG("ImageViewer viewer has finished with error: %s", qPrintable(errstr.right(300)));

        if(!errstr.isEmpty()) {
            MYDBG("   %s",  qPrintable(errstr));
        }
    }

    double length = (-1);

    if(MP->mediaInfo().has_length()) {
        length = MP->mediaInfo().length();
    }

    double lastpos = MP->last_position();

    if(lastpos < 0) { // we did not even start loading it yet
        lastpos = 0;
    }

    if(length >= 0 && lastpos > length) {
        lastpos = length;
    }

    MYDBG("movie finished. Played %f out of %f secs", lastpos, length);


    if(lastpos <= 0 || length <= 0) {
        qApp->quit();
    }

    const double diff = length - lastpos;
    const double rdiff = diff / length;

    if(diff > MIN_LASTPOS_DIFF_ABS_SECONDS || rdiff > MIN_LASTPOS_DIFF_REL_PERCENT) {
        MYDBG("lastpos pretty far from end, assuming user wants to quit");
        qApp->quit();
    }

    if(!success) {
        // (void) QMessageBox::warning(this, QLatin1String("Error showing movie"), output);
        qCritical("most recent movie player errord out");
        qApp->quit();
    }

    if(mfns.isEmpty()) {
        MYDBG("no more movies to show");
        qApp->quit();
    }

    MYDBG("queuing next movie");

    set_focus_raise(MP);

    activate_this_window();

    QTimer::singleShot(0, this, SLOT(slot_MP_start()));
}
void PlayerWindow::slot_MP_does_not_work()
{
    QString msg = QLatin1String("Can not execute mplayer, please look at the logs");
    qCritical() << msg;
    qApp->quit();
}
void PlayerWindow::slot_MP_error_while(QString reason, QString url, MpMediaInfo mmi, double lastpos)
{

    if(!url.isEmpty()) {
        qWarning("IV: MP error received: \"%s\", reloading again \"%s\" at %f", qPrintable(reason), qPrintable(url), lastpos);
        MP->load(url, mmi, lastpos);
    }
    else {
        qWarning("IV: MP error received: \"%s\"", qPrintable(reason));
    }
}

void PlayerWindow::slot_MP_state_changed(MpState oldstate, MpState newstate)
{
    Q_UNUSED(oldstate);
    qDebug("IV: MP state changed from %s to %s at %g/%g", convert_MpState_2_asciidesc(oldstate), convert_MpState_2_asciidesc(newstate), MP->current_position(), MP->mediaInfo().has_length() ? MP->mediaInfo().length() : (-1.));

    switch(newstate) {
        case MpState::NotStartedState:
            break;

        case MpState::IdleState:
            break;

        case MpState::LoadingState:
            break;

        case MpState::StoppedState:
            MP_finished(true);
            break;

        case MpState::PlayingState:
            MP_window_correct();
            break;

        case MpState::BufferingState:
            MP_window_correct();
            break;

        case MpState::PausedState:
            MP_window_correct();
            break;

        case MpState::ErrorState:
            // ignore - MP_error(str will be called)
            break;

        default:
            PROGRAMMERERROR("WTF");
    }

}

void PlayerWindow::MP_window_correct()
{
    MP->resize(this->size());
    MP->move(0, 0);
    set_focus_raise(MP);
}

void PlayerWindow::slot_cdDetected(bool success, QString msg, QString mfn)
{
    if(!success) {
        qWarning("detecting crop for \"%s\" did not succeed: %s", qPrintable(mfn), qPrintable(msg));
        return;
    }

    const QString &cropstring = msg;

    if(cropstring.isEmpty()) {
        MYDBG("\"%s\" does not need to be cropped", qPrintable(mfn));
        return;
    }

    if(mfn != currently_playing_mfn) {
        qWarning("got late crop \"%s\" for \"%s\" - discarding", qPrintable(cropstring), qPrintable(mfn));
        return;
    }

    MYDBG("got crop \"%s\" for \"%s\"", qPrintable(cropstring), qPrintable(mfn));
    MP->set_crop(cropstring);
}

void PlayerWindow::slot_MP_start()
{
    if(MP == NULL) {
        init_MP_object();
    }

    if(mfns.isEmpty()) {
        qApp->quit();
    }

    const QString absfn = mfns.takeFirst();

    QFileInfo qfi(absfn);

    setWindowTitle(qfi.fileName());

    MpMediaInfo mmi;
    mmi.set_seekable(true);
    // FIXME get deinterlace from command line or something

    currently_playing_mfn = absfn;

    const QByteArray bcropstring = qgetenv("CROP");

    if(bcropstring.isEmpty()) {
        if(cd != NULL) {
            delete cd;
            cd = NULL;
        }

        cd = new CropDetector(this, absfn);
        XCONNECT(cd, SIGNAL(sig_detected(bool, QString, QString)), this, SLOT(slot_cdDetected(bool, QString, QString)), QUEUEDCONN);
    }
    else {
        QString scropstring = QString::fromLocal8Bit(bcropstring.constData());
        mmi.set_crop(scropstring);
    }

    MP->load(absfn, mmi);
    MP_window_correct();
}

