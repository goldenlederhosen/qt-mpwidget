#include "system.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

#include "singleqprocesssingleshot.h"
#include "vregexp.h"
#include "config.h"
#include "encoding.h"

#define DEBUG_SYS

#ifdef DEBUG_ALL
#define DEBUG_SYS
#endif

#ifdef DEBUG_SYS
#define MYDBG(msg, ...) qDebug("SYS " msg, ##__VA_ARGS__)
#else
#define MYDBG(msg, ...)
#endif

static const int timeout_dvdrwmediainfo_msec = 3000;
static const int timeout_isoinfo_msec = 3000;
static const int timeout_blkid_msec = 3000;

static bool check_with_dvdrwmediainfo(const QString &dev, QStringList &erracc)
{
    QStringList drmargs;
    drmargs << dev;

    const QString exe = QLatin1String("dvd+rw-mediainfo");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_dvdrwmediainfo");
    QString error;

    if(!sqpss.run(exe, drmargs, timeout_dvdrwmediainfo_msec, error)) {
        erracc.append(error);
        return false;
    }

    return true;
}

static bool check_with_isoinfo(const QString &dev, QStringList &erracc)
{
    QStringList drmargs;
    drmargs << QLatin1String("-d");
    drmargs << QLatin1String("-i");
    drmargs << dev;

    const QString exe = QLatin1String("isoinfo");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_isoinfo_d_i");
    QString error;

    if(!sqpss.run(exe, drmargs, timeout_isoinfo_msec, error)) {
        erracc.append(error);
        return false;
    }

    return true;
}
static bool fs_looks_like_vdvd(const QString &dev, QStringList &erracc)
{
    QStringList drmargs;
    drmargs << QLatin1String("-f");
    drmargs << QLatin1String("-i");
    drmargs << dev;

    const QString exe = QLatin1String("isoinfo");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_isoinfo_f_i");
    QString error;

    if(!sqpss.run(exe, drmargs, timeout_isoinfo_msec, error)) {
        erracc.append(error);
        return false;
    }


    QString out = sqpss.get_output();
    QStringList lines = out.split(QLatin1Char('\n'));

    const QString VIDEOTS = QLatin1String("/VIDEO_TS/");
    foreach(const QString & l, lines) {
        if(l.indexOf(VIDEOTS) == 0) {
            return true;
        }
    }

    return false;
}

static bool dev_2_title(const QString &dev, QString &title, QStringList &erracc)
{
    QStringList drmargs;
    drmargs << QLatin1String("-s");
    drmargs << QLatin1String("LABEL");
    drmargs << QLatin1String("-o");
    drmargs << QLatin1String("value");
    drmargs << dev;

    const QString exe = QLatin1String("blkid");

    SingleQProcessSingleshot sqpss(NULL, "SQPS_blkid");
    QString error;

    if(!sqpss.run(exe, drmargs, timeout_blkid_msec, error)) {
        erracc.append(error);
        return false;
    }

    title = sqpss.get_output().simplified();

    return true;

}



bool find_dvddev(QString &retdev, QString &title, QString &body, QString &technical)
{
    QStringList candidates;

    candidates.append(QLatin1String("/dev/dvd"));
    candidates.append(QLatin1String("/dev/dvd0"));
    candidates.append(QLatin1String("/dev/dvd1"));
    candidates.append(QLatin1String("/dev/scd0"));
    candidates.append(QLatin1String("/dev/scd1"));
    candidates.append(QLatin1String("/dev/sr0"));
    candidates.append(QLatin1String("/dev/sr1"));

    // remove non-existing devices
    {
        const QString ocls = candidates.join(QLatin1String(", "));

        {
            QMutableListIterator<QString> i(candidates);

            while(i.hasNext()) {
                QString dev = i.next();

                if(!QFile::exists(dev)) {
                    i.remove();
                }
                else {
                    MYDBG("found device %s", qPrintable(dev));
                }
            }
        }

        if(candidates.isEmpty()) {
            title = QLatin1String("Drive not found");
            body = QLatin1String("No such devices ") + ocls;
            technical.clear();
            return false;
        }

    }

    //remove devices that map to the same abscanonpath
    {
        const QString ocls = candidates.join(QLatin1String(", "));

        {
            QSet<QString> seenabs;
            QMutableListIterator<QString> i(candidates);

            while(i.hasNext()) {
                QString dev = i.next();
                QFileInfo qfi(dev);
                QString abs = qfi.canonicalFilePath();

                if(abs.isEmpty()) {
                    i.remove();
                    qWarning("weird - canonicalFilePath can not see %s, but QFile::exists did?", qPrintable(dev));
                    continue;
                }

                if(seenabs.contains(abs)) {
                    i.remove();
                    MYDBG("remove duplicate canonpath %s -> %s", qPrintable(dev), qPrintable(abs));
                    continue;
                }

                seenabs.insert(abs);
            }
        }

        if(candidates.isEmpty()) {
            title = QLatin1String("Drive not found");
            body = QLatin1String("No such devices ") + ocls;
            technical.clear();
            return false;
        }
    }

    // remove devices without a DVD in them
    {
        const QString ocls = candidates.join(QLatin1String(", "));
        QStringList erracc;

        {
            QMutableListIterator<QString> i(candidates);

            while(i.hasNext()) {
                QString dev = i.next();

                if(!check_with_dvdrwmediainfo(dev, erracc)) {
                    i.remove();
                    MYDBG("no DVD in %s", qPrintable(dev));
                }
            }
        }

        if(candidates.isEmpty()) {
            title = QLatin1String("No DVD in drive?");
            body = QLatin1String("No DVD found in ") + ocls;
            technical = erracc.join(QLatin1String("\n"));
            return false;
        }
    }

    // remove devices where isoinfo fails
    {
        const QString ocls = candidates.join(QLatin1String(", "));
        QStringList erracc;

        {
            QMutableListIterator<QString> i(candidates);

            while(i.hasNext()) {
                QString dev = i.next();

                if(!check_with_isoinfo(dev, erracc)) {
                    i.remove();
                    MYDBG("isoinfo fails for %s", qPrintable(dev));
                }
            }
        }

        if(candidates.isEmpty()) {
            title = QLatin1String("Not a proper DVD?");
            body = QLatin1String("Could not see a proper DVD in ") + ocls;
            technical = erracc.join(QLatin1String("\n"));
            return false;
        }
    }

    // remove devices where the files do not look like a video dvd
    {
        const QString ocls = candidates.join(QLatin1String(", "));
        QStringList erracc;

        {
            QMutableListIterator<QString> i(candidates);

            while(i.hasNext()) {
                QString dev = i.next();

                if(!fs_looks_like_vdvd(dev, erracc)) {
                    i.remove();
                    MYDBG("files in %s do not look like a VIDEO DVD", qPrintable(dev));
                }
            }
        }

        if(candidates.isEmpty()) {
            title = QLatin1String("Not a video DVD?");
            body = QLatin1String("Could not see a video DVD in ") + ocls;
            technical = erracc.join(QLatin1String("\n"));
            return false;
        }
    }

    // if more than one left, construct error message including titles and bail
    if(candidates.size() > 1) {
        title = QLatin1String("Found multiple video DVDs");
        QStringList erracc;
        body = QLatin1String("Found video DVDs\n");
        foreach(const QString & dev, candidates) {
            QString t;

            if(dev_2_title(dev, t, erracc)) {
                body += QLatin1String("   ") + t + QLatin1String(" (") + dev + QLatin1String(")\n");
            }
            else {
                body += QLatin1String("   ") + dev + QLatin1String("\n");
            }
        }
        technical = erracc.join(QLatin1String("\n"));
        return false;
    }

    // success
    retdev = candidates.front();
    return true;

}

void add_exe_dir_to_PATH(char const *argv0)
{
    QSet<QString> dirs_to_add;
    {
        const QString dir1 = QCoreApplication::applicationDirPath();

        if(dir1.isEmpty()) {
            qWarning("QCoreApplication::applicationDirPath() returned empty string");
        }
        else {
            dirs_to_add.insert(dir1);
        }

        const QString dir2 = QFileInfo(err_xbin_2_local_qstring(argv0)).dir().path();

        if(!dir2.isEmpty() && (dir1 != dir2)) {
            dirs_to_add.insert(dir2);
        }
    }

    const QByteArray PATH_b = qgetenv("PATH");

    if(PATH_b.isEmpty()) {
        qWarning("qgetenv(PATH) returned empty string");
    }

    const QString PATH_s = err_xbin_2_local_qstring(PATH_b);
    QStringList PATH_l = PATH_s.split(QLatin1Char(':'), QString::SkipEmptyParts);
    PATH_l.removeDuplicates();

    bool found_missing = false;
    foreach(const QString & d, dirs_to_add) {
        if(!PATH_l.contains(d)) {
            PATH_l.append(d);
            found_missing = true;
        }
    }

    if(!found_missing) {
        return;
    }

    const QString new_PATH_s = PATH_l.join(QLatin1String(":"));
    const QByteArray new_PATH_b = new_PATH_s.toLocal8Bit();
    ::setenv("PATH", new_PATH_b.constData(), 1);
    MYDBG("export PATH=%s", new_PATH_b.constData());
}

static bool did_allow_core = false;
static QMutex mutex_did_allow_bool;

static bool get_did_allow_core()
{
    QMutexLocker l(&mutex_did_allow_bool);
    bool ret = did_allow_core;
    return ret;
}
static void set_did_allow_core()
{
    QMutexLocker l(&mutex_did_allow_bool);
    did_allow_core = true;
}

static void handle_abrt(const QString &cp)
{
    if(!cp.contains(QLatin1String("abrt"))) {
        return;
    }

    char const *const aaspd_fn = "/etc/abrt/abrt-action-save-package-data.conf";
    QFile aaspd_o((QLatin1String(aaspd_fn)));

    if(!aaspd_o.exists()) {
        qWarning("looks like abrtd is active but %s does not exist", aaspd_fn);
        return;
    }

    if(!aaspd_o.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("could not open %s", aaspd_fn);
        return;
    }

    VRegExp vr_bad("\\bProcessUnpackaged\\s*=\\s*no\\b");
    VRegExp vr_good("\\bProcessUnpackaged\\s*=\\s*yes\\b");

    QTextStream in(&aaspd_o);

    unsigned found = 0;

    while(true) {
        const QString line = in.readLine();

        if(line.isNull()) {
            return;
        }

        QString proc = line.trimmed();
        int hashpos = proc.indexOf(QLatin1Char('#'));

        if(hashpos >= 0) {
            proc.truncate(hashpos);
        }

        if(!proc.contains(QLatin1String("ProcessUnpackaged"))) {
            continue;
        }

        found++;

        if(proc.contains(vr_good)) {
            MYDBG("%s: %s - good", aaspd_fn, qPrintable(proc));
        }
        else if(proc.contains(vr_bad)) {
            qWarning("%s: %s - will not get core dumps", aaspd_fn, qPrintable(proc));
        }
        else {
            qWarning("%s: %s - I did not understand that line", aaspd_fn, qPrintable(proc));
        }
    }

    if(found == 0) {
        qWarning("did not find ProcessUnpackaged mentioned in %s - expected there to be a ..=yes line", aaspd_fn);
    }
    else if(found > 1) {
        qWarning("found %ud mentions of ProcessUnpackaged=... in %s - expected there to be exactly 1", found, aaspd_fn);
    }

}

void allow_core()
{
    if(get_did_allow_core()) {
        return;
    }

    set_did_allow_core();

    char const *const cp_fn = "/proc/sys/kernel/core_pattern";
    QFile f((QLatin1String(cp_fn)));

    if(f.exists()) {
        if(f.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&f);
            QString val = in.readAll();
            val = val.trimmed();
            MYDBG("%s: %s", cp_fn, qPrintable(val));
            handle_abrt(val);

        }
        else {
            qWarning("could not open %s file", cp_fn);
        }
    }

    struct rlimit rlim;

    rlim.rlim_cur = RLIM_INFINITY;

    rlim.rlim_max = RLIM_INFINITY;

    if(0 == ::setrlimit(RLIMIT_CORE, &rlim)) {
        MYDBG("did ulimit -c unlimited inside %s", qPrintable(QDir::currentPath()));
        return;
    }

    qWarning("could not set ulimit -c unlimited: %s", strerror(errno));
}

bool definitely_running_from_desktop()
{
    if(setand1_getenv("FROMDESKTOP")) {
        MYDBG("FROMDESKTOP set - assuming run from desktop");
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

        MYDBG("%s", qPrintable(msg));
    }

    if(found_tty) {
        MYDBG("Looks like we are running from the command line");
        return false;
    }
    else {

        MYDBG("Looks like we are running from the desktop");
        return true;
    }
}

static bool looks_like_vdpau()
{
    // if vdpauinfo exists: call it. if return value = 0 - true
    int ret_vdpauinfo = QProcess::execute(QLatin1String("vdpauinfo"));

    if(ret_vdpauinfo == 0) {
        MYDBG("vdpauinfo executed successfully, guessing nvidia");
        return true;
    }
    else if(ret_vdpauinfo > 0) {
        MYDBG("vdpauinfo failed, no nvidia");
        return false;
    }
    else {
        MYDBG("vdpauinfo not found/crashed");
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
                MYDBG("xdpyinfo mentioned NV-CONTROL and NV-GLX, guessing nvidia");
                return true;
            }
        }
    }

    if(xdpyinfo_succeeded) {
        MYDBG("xdpyinfo did not mention NV-CONTROL and NV-GLX");
    }
    else {
        qWarning("xdpyinfo did not succeed, this is weird");
    }

    return false;
}

static bool looks_like_nomachine()
{
    const QByteArray IN_NX = qgetenv("IN_NX");

    if(!IN_NX.isEmpty() && IN_NX == "1") {
        MYDBG("found IN_NX=1, assuming nomachine");
        return true;
    }

    const QByteArray SSH_ORIGINAL_COMMAND = qgetenv("SSH_ORIGINAL_COMMAND");

    if(!SSH_ORIGINAL_COMMAND.isEmpty() && SSH_ORIGINAL_COMMAND.endsWith("/nxnode")) {
        MYDBG("found SSH_ORIGINAL_COMMAND=.../nxnode, assuming nomachine");
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
                MYDBG("glxinfo mentioned \" on AMD \", guessing radeon");
                return true;
            }
        }
    }

    if(glxinfo_succeeded) {
        MYDBG("glxinfo did not mention \" on AMD \"");
    }
    else {
        qWarning("glxinfo did not succeed, this is weird");
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
                MYDBG("glxinfo mentioned \"direct rendering: Yes\", guessing OpenGL is there");
                return true;
            }
        }
    }

    if(glxinfo_succeeded) {
        MYDBG("glxinfo did not mention \"direct rendering: Yes\"");
    }
    else {
        qWarning("glxinfo did not succeed, this is weird");
    }

    return false;
}

QString compute_MP_VO()
{
    const QString VO_set = get_MP_VO();

    if(!VO_set.isEmpty()) {
        return VO_set;
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

static QString compute_VDB_RUN_from_HOME(QString *reason)
{
    QString HOME = QDir::homePath();

    if(HOME.isEmpty()) {
        if(reason != NULL) {
            *reason = QLatin1String("HOME directory not found");
        }

        QString empty;
        return empty;
    }

    HOME.append(QLatin1String("/.vdb"));
    return HOME;
}

QString compute_VDB_RUN(QString *reason)
{

    QString vdb_run_s = get_VDB_RUN();

    if(vdb_run_s.isEmpty()) {
        vdb_run_s = compute_VDB_RUN_from_HOME(reason);

        if(vdb_run_s.isEmpty()) {
            return vdb_run_s;
        }
    }

    QDir vdb_run_o(vdb_run_s);

    if(!vdb_run_o.exists()) {
        if(reason != NULL) {
            *reason = QLatin1String("VDB_RUN ") + vdb_run_s + QLatin1String(" does not exist");
        }

        qWarning("VDB_RUN %s does not exist", qPrintable(vdb_run_s));
        QString empty;
        return empty;
    }

    if(reason != NULL) {
        reason->clear();
    }

    MYDBG("computed VDB_RUN=%s", qPrintable(vdb_run_s));

    return vdb_run_s;
}

