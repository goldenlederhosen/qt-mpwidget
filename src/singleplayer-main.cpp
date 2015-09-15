#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QSet>
#include <QLatin1String>
#include <QHash>
#include <QCommandLineParser>
#include <QLoggingCategory>

#include "mainwindow.h"
#include "capslock.h"
#include "system.h"

//#include <QLoggingCategory>
//#define THIS_SOURCE_FILE_LOG_CATEGORY "MAIN"
//static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
//#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static void usage(const QString &in_msg = QString())
{
    // fixme fataldialog
    QString msg = in_msg;

    if(!msg.isEmpty()) {
        msg.append(QLatin1String("\n"));
    }

    qCritical("%sUSAGE: %s <fullscreen|window> movfn.....\n"
              "\n"
              "--forbidden-alang=lang3  - forbidden audio language (may be multiple)\n"
              "--pref-alang=lang3       - preferred audio language (may be multiple)\n"
              "--pref-slang=lang3       - preferred subtitle language (may be multiple)\n"
              "\n"
              "MP_OPTS_APPEND   - extra mplayer command line options\n"
              "MP_OPTS_OVERRIDE - mplayer command line options\n"
              "MP_VO            - mplayer -vo option\n"
              "CROP             - mplayer-like crop string\n"
              "QT_LOGGING_RULES - change default logging"
              "\n"
              "Booleans:\n"
              "SIL_MEASURE_LATENCY\n"
              "SIL_SLEEP\n"
              "IV_NO_EPISODE_AUTOADVANCE\n"
              "MC_DO_SHOW_VIDEO_SIZE\n"
              "MC_FULL_TAGS\n"
              , qPrintable(msg)
              , qPrintable(qApp->applicationFilePath())
             );
}
static void usage(char const *const cmsg)
{
    usage(QLatin1String(cmsg));
}

void parse_commandline(QCoreApplication &app
                       , QStringList &mfns
                       , bool &fullscreen
                       , QStringList &falangs
                       , QStringList &palangs
                       , QStringList &pslangs)
{

    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("Single Player"));
    parser.addHelpOption();

    QCommandLineOption opt_falang(QStringList()  << QLatin1String("forbidden-alang")
                                  , QLatin1String("forbidden audio language.")
                                  , QLatin1String("lang3")
                                 );
    parser.addOption(opt_falang);

    QCommandLineOption opt_palang(QStringList()  << QLatin1String("pref-alang")
                                  , QLatin1String("preferred audio language.")
                                  , QLatin1String("lang3")
                                 );
    parser.addOption(opt_palang);

    QCommandLineOption opt_pslang(QStringList() << QLatin1String("s") << QLatin1String("pref-slang")
                                  , QLatin1String("preferred subtitle language.")
                                  , QLatin1String("lang3")
                                 );
    parser.addOption(opt_pslang);

    parser.process(app);

    falangs = parser.values(opt_falang);
    palangs = parser.values(opt_palang);
    pslangs = parser.values(opt_pslang);

    {
        const QStringList uon = parser.unknownOptionNames();

        if(!uon.isEmpty()) {
            QString uon_s = QLatin1String("found unknown switches ") + uon.join(QLatin1String(", "));
            usage(uon_s);
            exit(1);
        }
    }

    QStringList pa = parser.positionalArguments();

    if(pa.isEmpty()) {
        usage("no movie files mentioned");
        exit(1);
    }

    const QString &smode = pa[0];

    if(smode == QLatin1String("fullscreen")) {
        fullscreen = true;
        pa.removeFirst();
    }
    else if(smode == QLatin1String("window")) {
        fullscreen = false;
        pa.removeFirst();
    }
    else {
        fullscreen = false;
    }

    if(pa.isEmpty()) {
        usage("no movie files mentioned");
        exit(1);
    }

    mfns = pa;

}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // override with QT_LOGGING_RULES
    QLoggingCategory::setFilterRules(QStringLiteral(
                                         "*.debug=false\n"
                                         "BADLOG=true\n"
                                         "CAPSLOCK=true\n"
                                         "CFG=true\n"
                                         "DBUS=true\n"
                                         "ENCODING=true\n"
                                         "EC=true\n"
                                         "IV=true\n"
                                         "OQ=true\n"
                                         "SL=true\n"
                                         "MAIN=true\n"
                                         "MMPIMP=true\n"
                                         "REMLOC=true\n"
                                         "SSMN=true\n"
                                         "SYS=true\n"
                                         "UTIL=true\n"
                                     ));

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    if(definitely_running_from_desktop()) {
        qInstallMessageHandler(desktopMessageOutput);
    }
    else {
        qInstallMessageHandler(commandlineMessageOutput);
    }

#else

    if(definitely_running_from_desktop()) {
        qInstallMsgHandler(desktopMessageOutput);
    }
    else {
        qInstallMsgHandler(commandlineMessageOutput);
    }

#endif

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

    add_exe_dir_to_PATH(argv[0]);
    allow_core();
    QCoreApplication::setOrganizationName(QLatin1String("EckeSoft"));
    QCoreApplication::setApplicationName(QLatin1String("SinglePlayer"));

    if(checkCapsLock()) {
        // fixme fataldialog
        qCritical("CAPSLOCK down");
        return(1);
    }

    (void)qRegisterMetaType<MpMediaInfo>();

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    QStringList mfns;
    bool fullscreen = false;
    QStringList falangs;
    QStringList palangs;
    QStringList pslangs;
    parse_commandline(app, mfns, fullscreen, falangs, palangs, pslangs);

    PlayerWindow *player = new PlayerWindow(fullscreen, mfns, falangs, palangs, pslangs);

    player->show();
    int ret = app.exec();
    delete player;
    player = NULL;
    qWarning("ret=%d", ret);
    return ret;
}
