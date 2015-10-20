#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QSet>
#include <QLatin1String>
#include <QHash>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QMutex>
#include <signal.h>

#include "mainwindow.h"
#include "capslock.h"
#include "system.h"
#include "logging.h"
#include "event_desc.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "MAIN"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

static_var PlayerWindow *player = NULL;
static_var QMutex playerlock;

static void do_cleanup_moviechooser(char const *const from)
{
    if(!playerlock.tryLock(500)) { // miliseconds
        return;
    }

    if(player == NULL) {
        MYDBG("%s: player is NULL", from);
        playerlock.unlock();
        return;
    }

    MYDBG("%s: deleting moviechooser", from);
    PlayerWindow *copy = player;
    player = NULL;

    playerlock.unlock();

    delete copy;
}

static void do_cleanup_moviechooser_atexit()
{
    do_cleanup_moviechooser("atexit");
}
static void do_cleanup_moviechooser_autocum(const QString &desc)
{
    do_cleanup_moviechooser(qPrintable(desc));
}
static void do_cleanup_moviechooser_ex()
{
    do_cleanup_moviechooser("exception");
}
static void do_cleanup_moviechooser_exel()
{
    do_cleanup_moviechooser("exceptionellipsis");
}
static void do_cleanup_moviechooser_end()
{
    do_cleanup_moviechooser("end");
}
static void do_cleanup_moviechooser_terminate()
{
    do_cleanup_moviechooser("terminate");
    exit(1);
}
void do_cleanup_moviechooser_sig(int signum)
{
    char const *const sigdesc = strsignal(signum);
    char *msg = NULL;
    const int ret = asprintf(&msg, "captured signal \"%s\"", sigdesc);

    if(ret > 0 && msg != NULL) {
        do_cleanup_moviechooser(msg);
        free(msg);
        msg = NULL;
    }
    else {
        do_cleanup_moviechooser(sigdesc);
    }

    raise(signum);
}
void set_signal(int signo)
{
    struct sigaction new_action, old_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = do_cleanup_moviechooser_sig;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = SA_RESETHAND;

    if(0 != sigaction(signo, &new_action, &old_action)) {
        qWarning("could not install signal handler for %s: %s", strsignal(signo), strerror(errno));
    }
    else {
        char const *const sigdesc = strsignal(signo);
        MYDBG("installed signal handler for %s", sigdesc);
    }
}



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

class AutoCUM
{
private:
    const QString m_desc;
public:
    AutoCUM(const QString &in_desc): m_desc(in_desc) {}
    AutoCUM(char const *const in_desc_latin1): m_desc(QLatin1String(in_desc_latin1)) {}
    ~AutoCUM()
    {
        do_cleanup_moviechooser_autocum(m_desc);
    }
};

static AutoCUM autocum_static("autocum_static");


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    init_logging();

    init_signals_spy();

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

    (void) atexit(do_cleanup_moviechooser_atexit);
    AutoCUM _autocum("autocum-stack");
    std::set_terminate(do_cleanup_moviechooser_terminate);

    if(setand1_getenv("MC_NO_CAPTURE_SIGS")) {
        MYDBG("NOT setting signal handlers");
    }
    else {
        MYDBG("setting signal handlers");
        set_signal(SIGHUP);
        set_signal(SIGINT);
        set_signal(SIGQUIT);
        set_signal(SIGILL);
        set_signal(SIGBUS);
        set_signal(SIGFPE);
        set_signal(SIGSEGV);
        set_signal(SIGPIPE);
        set_signal(SIGTERM);
        set_signal(SIGXCPU);
        set_signal(SIGIO);
    }

    int ret = -1;

    try {
        {
            QMutexLocker locker(&playerlock);
            player = new PlayerWindow(fullscreen, mfns, falangs, palangs, pslangs);
        }


        player->show();
        ret = app.exec();
    }

    catch(std::exception &e) {
        do_cleanup_moviechooser_ex();
        fprintf(stderr, "main program: caught exception %s", e.what());
        return 1;
    }
    catch(...) {
        do_cleanup_moviechooser_exel();
        fprintf(stderr, "main program: caught exception");
        return 1;
    }

    do_cleanup_moviechooser_end();

    qWarning("ret=%d", ret);
    return ret;
}
