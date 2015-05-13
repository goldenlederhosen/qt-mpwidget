#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QSet>
#include <QLatin1String>
#include <QHash>

#include "mainwindow.h"
#include "capslock.h"

//#define DEBUG_MAIN

#ifdef DEBUG_ALL
#define DEBUG_MAIN
#endif

#ifdef DEBUG_MAIN
#define MYDBG(msg, ...) qDebug("MAIN " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

static void usage()
{
    // fixme fataldialog
    qCritical("USAGE: %s <fullscreen|window> movfn.....\n"
              "\n"
              "MP_PALANGS       - preferred audio languages comma separated list\n"
              "MP_PSLANGS       - preferred subtitle languages comma separated list\n"
              "MP_OPTS_APPEND   - extra mplayer command line options\n"
              "MP_OPTS_OVERRIDE - mplayer command line options\n"
              "MP_VO            - mplayer -vo option\n"
              "\n"
              "Booleans:\n"
              "SIL_MEASURE_LATENCY\n"
              "SIL_SLEEP\n"
              "IV_NO_EPISODE_AUTOADVANCE\n"
              "MC_DO_SHOW_VIDEO_SIZE\n"
              "MC_FULL_TAGS\n"
              "QMP_PRINT_CRAP_OUTPUT\n"
              "QMP_PRINT_STATUS_OUTPUT\n"
              , qPrintable(qApp->applicationFilePath())
             );
}

static QStringList charstarstar_2_stringlist(char const *const *const argv, int argc)
{
    QStringList l;

    for(int i = 0; i < argc; i++) {
        l.append(QString::fromLocal8Bit(argv[i]));
    }

    return l;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if(definitely_running_from_desktop()) {
        qInstallMsgHandler(desktopMessageOutput);
    }
    else {
        qInstallMsgHandler(commandlineMessageOutput);
    }

    if(argc < 2) {
        usage();
        return(1);
    }

    for(int i = 1; i < argc; i++) {
        if(
            (strcmp(argv[i], "-h") == 0)
            || (strcmp(argv[i], "-help") == 0)
            || (strcmp(argv[i], "--help") == 0)
            || (strcmp(argv[i], "-?") == 0)
        ) {
            usage();
            return(1);
        }
    }

    if(checkCapsLock()) {
        // fixme fataldialog
        qCritical("CAPSLOCK down");
        return(1);
    }

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    bool fullscreen = false;

    int starti = 1;
    QString farg = QString::fromLocal8Bit(argv[1]);

    if(farg == QLatin1String("fullscreen")) {
        fullscreen = true;
        starti++;
    }
    else if(farg == QLatin1String("window")) {
        fullscreen = false;
        starti++;
    }

    QStringList mfns = charstarstar_2_stringlist(argv + starti, argc - starti);

    if(mfns.isEmpty()) {
        // fixme fataldialog
        qCritical("Could not find valid dirs??");
        return(1);
    }

    (void)qRegisterMetaType<MpMediaInfo>();

    PlayerWindow *player = new PlayerWindow(fullscreen, mfns);

    player->show();
    int ret = app.exec();
    delete player;
    player = NULL;
    qWarning("ret=%d", ret);
    return ret;
}
