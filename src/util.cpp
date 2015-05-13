#include <sys/types.h>
#include <unistd.h>
#include <QDebug>
#include <QApplication>
#include <QProcess>

#include "gui_overlayquit.h"
#include "util.h"
#include "keepfocus.h"

QString dt_2_human(const QDateTime &lastseenon)
{
    QString ds;
    QDateTime now = QDateTime::currentDateTime();
    QDate nowd = now.date();
    QDate lsnd = lastseenon.date();
    const int daysago = lsnd.daysTo(nowd);

    if(daysago == 0) {
        ds = QLatin1String("today");
    }
    else if(daysago == 1) {
        ds = QLatin1String("yesterday");
    }
    else if(daysago < 6) {
        ds = lastseenon.toString(QLatin1String("dddd"));
    }
    else if(nowd.year() != lsnd.year()) {
        ds = lastseenon.toString(QLatin1String("d MMMM yyyy"));
    }
    else if(nowd.month() != lsnd.month()) {
        ds = lastseenon.toString(QLatin1String("d MMMM"));
    }
    else if(nowd.day() != lsnd.day()) {
        ds = lastseenon.toString(QLatin1String("dddd d MMMM"));
    }
    else {
        ds = QLatin1String("today");
    }

    return ds;
}

QString tlenpos_2_human(double dsec)
{
    if(dsec > 60 * 60 * 24 * 10) { // 10 days
        PROGRAMMERERROR("time length %f sec much too large", dsec);
    }

    if(dsec < 0) {
        PROGRAMMERERROR("time length %f sec negative", dsec);
    }

    double rsecs = round(dsec);

    if(rsecs < 1) {
        return QLatin1String("00:00:00");
    }

    if(rsecs >= SIZE_MAX) {
        return QLatin1String("99:99:99");
    }

    size_t secs = rsecs;
    size_t mins = secs / 60;
    secs -= mins * 60;
    size_t hours = mins / 60;
    mins -= hours * 60;
    size_t days = hours / 24;
    hours -= days * 24;

    QString ts;
    ts.sprintf("%02u:%02u:%02u", unsigned(hours), unsigned(mins), unsigned(secs));

    if(days > 10) {
        PROGRAMMERERROR("WTF");
    }

    if(days > 0) {
        ts.prepend(QString::number(days) + QLatin1String(" days "));
    }

    return ts;
}

static inline bool isWhiteSpace(ushort c)
{
    return c == ' ' || c == '\t';
}

QStringList doSplitArgs(const QString &args)
{
    QStringList ret;

    int p = 0;
    const int length = args.length();
    forever {
        forever {
            if(p == length) {
                return ret;
            }

            if(!isWhiteSpace(args.unicode()[p].unicode())) {
                break;
            }

            ++p;
        }

        QString arg;
        bool inquote = false;
        forever {
            bool copy = true; // copy this char
            int bslashes = 0; // number of preceding backslashes to insert

            while(p < length && args.unicode()[p] == QLatin1Char('\\')) {
                ++p;
                ++bslashes;
            }

            if(p < length && args.unicode()[p] == QLatin1Char('"')) {
                if(!(bslashes & 1)) {
                    // Even number of backslashes, so the quote is not escaped.
                    if(inquote) {
                        if(p + 1 < length && args.unicode()[p + 1] == QLatin1Char('"')) {
                            // This is not documented on MSDN.
                            // Two consecutive quotes make a literal quote. Brain damage:
                            // this still closes the quoting, so a 3rd quote is required,
                            // which makes the runtime's quoting run out of sync with the
                            // shell's one unless the 2nd quote is escaped.
                            ++p;
                        }
                        else {
                            // Closing quote
                            copy = false;
                        }

                        inquote = false;
                    }
                    else {
                        // Opening quote
                        copy = false;
                        inquote = true;
                    }
                }

                bslashes >>= 1;
            }

            while(--bslashes >= 0) {
                arg.append(QLatin1Char('\\'));
            }

            if(p == length || (!inquote && isWhiteSpace(args.unicode()[p].unicode()))) {
                ret.append(arg);

                if(inquote) {
                    return QStringList();
                }

                break;
            }

            if(copy) {
                arg.append(args.unicode()[p]);
            }

            ++p;
        }
    }
    //not reached
}

bool setand1_getenv(char const *const varname)
{
    const QByteArray envval = qgetenv(varname);

    if(envval.isEmpty()) {
        return false;
    }

    if(envval.size() != 1) {
        return false;
    }

    if(envval.at(0) != '1') {
        return false;
    }

    return true;
}

bool definitely_running_from_desktop()
{
    if(setand1_getenv("FROMDESKTOP")) {
        qDebug("FROMDESKTOP set - assuming run from desktop");
        return true;
    }

    bool found_tty = false;
    const pid_t pid = getpid();

    for(int fd = 0; fd < 3; fd++) {
        QString msg = QLatin1String("FD %1: ");
        msg = msg.arg(fd);

        {

            QLatin1String format("/proc/%1/fd/%2");
            QString qtpath = QString(format).arg(pid).arg(fd);
            char *cpath = strdup(qtpath.toLocal8Bit().constData());
            char readlink_buf[256];
            ssize_t readlink_ret = readlink(cpath, readlink_buf, 256);
            free(cpath);
            cpath = NULL;
            readlink_buf[255] = '\0';
            QString readlinkmsg;

            if(readlink_ret == (-1)) {
                // normal error - see errno
                readlinkmsg = QLatin1String("could not readlink(%1): %1");
                readlinkmsg = readlinkmsg.arg(qtpath).arg(warn_xbin_2_local_qstring(strerror(errno)));
            }
            else if(readlink_ret == 0) {
                // nothing copied?
                readlinkmsg = QLatin1String("could not readlink(%1): nothing copied");
                readlinkmsg = readlinkmsg.arg(qtpath);
            }
            else if(readlink_ret < 0) {
                // should never happen
                readlinkmsg = QLatin1String("could not readlink(%1): unknown error");
                readlinkmsg = readlinkmsg.arg(qtpath);
            }
            else {
                // readlink_ret contains number of bytes placed in buffer
                readlink_buf[readlink_ret] = '\0';
                readlinkmsg = QLatin1String("readlink(%1)=%2");
                readlinkmsg = readlinkmsg.arg(qtpath).arg(warn_xbin_2_local_qstring(readlink_buf));
            }

            msg.append(readlinkmsg);
        }

        {

            msg.append(QLatin1Char(' '));

            // /proc/$$/fd/$FD - is a link
            int isatty_ret = isatty(fd);

            if(isatty_ret == 1) {
                msg.append(QLatin1String("is a terminal"));
                found_tty = true;
            }
            else if(isatty_ret != 0) {
                QLatin1String format("isatty() returned %1 - should only be 1 or 0. Assuming means false");
                QString isattymsg = QString(format).arg(isatty_ret);
                msg.append(isattymsg);
            }
            else {

                // errno - EBADF - not a valid FD; EINVAL or ENOTTY - not a terminal
                if(errno == EBADF) {
                    msg.append(QLatin1String("is not open"));
                }
                else if(errno == EINVAL || errno == ENOTTY) {
                    msg.append(QLatin1String("is not a terminal"));
                }
                else {
                    msg.append(QLatin1String("is not a terminal: "));
                    msg.append(warn_xbin_2_local_qstring(strerror(errno)));
                }
            }
        }

        qDebug("%s", qPrintable(msg));
    }

    if(found_tty) {
        qDebug("Looks like we are running from the command line");
        return false;
    }
    else {

        qDebug("Looks like we are running from the desktop");
        return true;
    }
}

void desktopMessageOutput(QtMsgType type, const char *msg)
{
    //const bool isPE=( strstr(msg,"PROGRAMMERERROR")!=NULL );

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warning: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = focus_should_currently_be_at();

            if(mainw != NULL) {
                QLatin1String format("Critical: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->setObjectName(QLatin1String("OQ crit"));
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Critical: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            QWidget *mainw = QApplication::activeWindow();
            QWidget *foc = focus_should_currently_be_at();

            if(mainw != NULL) {
                QLatin1String format("Fatal: %1");
                QString qmsg = QString(format).arg(warn_xbin_2_local_qstring(msg));
                OverlayQuit *oq = new OverlayQuit(mainw, qmsg);
                XCONNECT(oq, SIGNAL(sig_stopped()), oq, SLOT(deleteLater()), QUEUEDCONN);
                oq->setObjectName(QLatin1String("OQ fatal"));
                oq->set_ori_focus(foc);
                oq->start_oq();
            }

            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}
void commandlineMessageOutput(QtMsgType type, const char *msg)
{
    //const bool isPE=( strstr(msg,"PROGRAMMERERROR")!=NULL );

    switch(type) {
        case QtDebugMsg: {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;

        case QtWarningMsg: {
            fprintf(stderr, "Warni: %s\n", msg);
        }
        break;

        case QtCriticalMsg: {
            fprintf(stderr, "Criti: %s\n", msg);
        }
        break;

        case QtFatalMsg: {
            fprintf(stderr, "Fatal: %s\n", msg);
            exit(1);
        }
        break;

        default: {
            fprintf(stderr, "Unknown output type: %s\n", msg);
        }
        break;
    }
}

static bool looks_like_vdpau()
{
    // if vdpauinfo exists: call it. if return value = 0 - true
    int ret_vdpauinfo = QProcess::execute(QLatin1String("vdpauinfo"));

    if(ret_vdpauinfo == 0) {
        qDebug("vdpauinfo executed successfully, guessing nvidia");
        return true;
    }
    else if(ret_vdpauinfo > 0) {
        qDebug("vdpauinfo failed, no nvidia");
        return false;
    }
    else {
        qDebug("vdpauinfo not found/crashed");
    }

    // call xdpyinfo. grep for NV-CONTROL and NV-GLX
    QProcess xdpyinfo;
    xdpyinfo.start(QLatin1String("xdpyinfo"));
    bool xdpyinfo_succeeded = false;

    if(xdpyinfo.waitForStarted()) {

        if(xdpyinfo.waitForFinished()) {
            xdpyinfo_succeeded = true;
            QByteArray result = xdpyinfo.readAll();

            if(result.indexOf("NV-CONTROL") >= 0 && result.indexOf("NV-GLX") >= 0) {
                qDebug("xdpyinfo mentioned NV-CONTROL and NV-GLX, guessing nvidia");
                return true;
            }
        }
    }

    if(xdpyinfo_succeeded) {
        qDebug("xdpyinfo did not mention NV-CONTROL and NV-GLX");
    }
    else {
        qDebug("xdpyinfo did not succeed, this is weird");
    }

    return false;
}

static bool looks_like_nomachine()
{
    const QByteArray IN_NX = qgetenv("IN_NX");

    if(!IN_NX.isEmpty() && IN_NX == "1") {
        qDebug("found IN_NX=1, assuming nomachine");
        return true;
    }

    const QByteArray SSH_ORIGINAL_COMMAND = qgetenv("SSH_ORIGINAL_COMMAND");

    if(!SSH_ORIGINAL_COMMAND.isEmpty() && SSH_ORIGINAL_COMMAND.endsWith("/nxnode")) {
        qDebug("found SSH_ORIGINAL_COMMAND=.../nxnode, assuming nomachine");
        return true;
    }

    return false;
}

static bool looks_like_radeon()
{
    QProcess glxinfo;
    glxinfo.start(QLatin1String("glxinfo"));
    bool glxinfo_succeeded = false;

    if(glxinfo.waitForStarted()) {

        if(glxinfo.waitForFinished()) {
            glxinfo_succeeded = true;
            QByteArray result = glxinfo.readAll();

            if(result.indexOf(" on AMD ") >= 0) {
                qDebug("glxinfo mentioned \" on AMD \", guessing radeon");
                return true;
            }
        }
    }

    if(glxinfo_succeeded) {
        qDebug("glxinfo did not mention \" on AMD \"");
    }
    else {
        qDebug("glxinfo did not succeed, this is weird");
    }

    return true;
}

static bool accelerated_gl()
{
    QProcess glxinfo;
    glxinfo.start(QLatin1String("glxinfo"));
    bool glxinfo_succeeded = false;

    if(glxinfo.waitForStarted()) {

        if(glxinfo.waitForFinished()) {
            glxinfo_succeeded = true;
            QByteArray result = glxinfo.readAll();

            if(result.indexOf("direct rendering: Yes") >= 0) {
                qDebug("glxinfo mentioned \"direct rendering: Yes\", guessing OpenGL is there");
                return true;
            }
        }
    }

    if(glxinfo_succeeded) {
        qDebug("glxinfo did not mention \"direct rendering: Yes\"");
    }
    else {
        qDebug("glxinfo did not succeed, this is weird");
    }

    return false;
}

QString get_MP_VO()
{
    const QByteArray VO = qgetenv("MP_VO");

    if(!VO.isEmpty()) {
        return err_xbin_2_local_qstring(VO);
    }

    if(looks_like_vdpau()) {
        return QLatin1String("vdpau:hqscaling=1:deint=4,gl:lscale=1:cscale=1,xv,x11");
    }

    if(looks_like_nomachine()) {
        return QLatin1String("x11");
    }

    if(looks_like_radeon()) {
        // mpv: lscale=lanczos2:dither-depth=auto:fbo-format=rgb16
        return QLatin1String("gl:lscale=1:cscale=1,xv,x11");
    }

    if(accelerated_gl()) {
        return QLatin1String("gl:lscale=1:cscale=1,xv,x11");
    }

    return QLatin1String("x11");

}
