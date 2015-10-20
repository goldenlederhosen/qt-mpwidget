#include <QFile>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QFileInfo>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <new>

#include "asyncreadfile.h"
#include "asyncreadfile_child.h"
#include "encoding.h"
#include "asynckillproc.h"
#include "util.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "CLF"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

#ifdef CLF_DEBUG
static_var const bool CLF_DEBUG_enabled = true;
#else
static_var const bool CLF_DEBUG_enabled = false;
#endif

#define ASFMYDBG(msg, ...) do{if(CLF_DEBUG_enabled){fprintf(stderr, "CLF P  %s " msg "\n", qPrintable(m_ofn.right(20)), ##__VA_ARGS__);}}while(0)
#define CLDMYDBG(msg, ...) do{if(CLF_DEBUG_enabled){fprintf(stderr, "CLF C  %s PID=%d " msg "\n", c_filename_short, getpid(), ##__VA_ARGS__);}}while(0)

AsyncReadFile::AsyncReadFile(const QString &in_fn, bool in_getc, off_t in_maxreadsize):
    m_ofn(in_fn)
    , m_getc(in_getc)
    , m_maxreadsize(in_maxreadsize)
    , c_filename(NULL)
    , m_done_pipefork(false)
    , m_msgfd(-1)
    , m_contentfd(-1)
    , m_pid(0)
    , m_done(false)
    , errmsg_bufsize(16384)
    , fileread_bufsize(65536)
    , pipe_bufsize(16384)
    , m_errmsg_buffer(NULL)
    , m_content_buffer_pipe_from_child(NULL)
{
}

QString AsyncReadFile::make_latin1_errmsg(int errno_copy, char const *const fmt, ...)  const
{
    static_var const QString inbetw((QStringLiteral(": ")));

    QString s;
    const int maxl = 20;

    if(m_ofn.length() > maxl) {
        s = QStringLiteral("...");
        const int l = s.length();
        s.append(m_ofn.right(maxl - l));
    }
    else {
        s = m_ofn;
    }

    s.append(inbetw);

    if(strchr(fmt, '%') == NULL) {
        s.append(QLatin1String(fmt));
    }
    else {
        QString qfmt;
        va_list ap;
        va_start(ap, fmt);
        qfmt.vsprintf(fmt, ap);
        va_end(ap);
        s.append(qfmt);
    }

    if(errno_copy != 0) {
        QString errstr(warn_xbin_2_local_qstring(strerror(errno_copy)));
        s.append(inbetw);
        s.append(errstr);
    }

    return s;
}

// check that we can load the first N bytes of the file. This takes some time
// (on the order of 30 MB/sec), but since it will just put those blocks
// into the block cache it will speed the mplayer startup by that much.
// On the flipside it gives us better diagnostics

void AsyncReadFile::force_kill_child(char const *const fmt, ...)
{
    if(m_pid > 0) {
        const pid_t oripid = m_pid;
        m_pid = 0;

        if(::strchr(fmt, '%') == NULL) {
            async_kill_process(oripid, fmt, qPrintable(m_ofn.right(20)));
        }
        else {
            va_list ap;
            va_start(ap, fmt);
            char *kreason = NULL;
            const int byteswritten = ::vasprintf(&kreason, fmt, ap);

            if(byteswritten <= 0 || kreason == NULL) {
                va_end(ap);
                throw std::bad_alloc();
            }

            va_end(ap);

            async_kill_process(oripid, kreason, qPrintable(m_ofn.right(20)));
            ::free(kreason);
        }

    }

}

#define PIPE_READ 0
#define PIPE_WRITE 1

bool AsyncReadFile::xpipe2(int *fds, char const *const desc, QStringList *errors)
{
    if(::pipe2(fds, O_NONBLOCK | O_CLOEXEC) == -1) {
        errors->append(make_latin1_errmsg(errno, "could not pipe2 %s", desc));
        return false;
    }
    else {
        ASFMYDBG("pipe2() -> %s read FD=%d write FD=%d", desc, fds[PIPE_READ], fds[PIPE_WRITE]);
        return true;
    }
}

bool AsyncReadFile::xclose(int &fd, char const *const desc)
{
    (void)desc;

    if(fd >= 0) {

        const int orifd = fd;
        fd = (-1);

        if(::close(orifd)) {
            ASFMYDBG("close(%s FD=%d): %s", desc, orifd, strerror(errno));
            return false;
        }
        else {
            ASFMYDBG("close(%s FD=%d)", desc,  orifd);
            return true;
        }
    }

    return true;
}

bool AsyncReadFile::xread(int &fd, char const *const desc, char *buffer, size_t bufsize, QStringList *errors, QByteArray *accumulator, bool *got_bytes)
{
    *got_bytes = false;
    ssize_t rcount = ::read(fd, buffer, bufsize);

    if(rcount < 0) {
        // error
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            //ASFMYDBG("read(%s FD=%d) = %ld, EAGAIN", descm (long)msgrcount);
            // fine
        }
        else {
            ASFMYDBG("read(errmsg FD=%d) = %ld %s", fd, (long)rcount, strerror(errno));
            force_kill_child("due to read error on %s FD", desc);
            errors->append(make_latin1_errmsg(errno, "read error on %s FD", desc));
            return false;
        }
    }
    else if(rcount == 0) {
        // EOF
        ASFMYDBG("got EOF from child on %s FD=%d", desc, fd);
        (void)xclose(fd, desc);
    }
    else {
        ASFMYDBG("got %lu bytes from child FD=%d", (unsigned long)rcount, fd);
        accumulator->append(buffer, rcount);
        *got_bytes = true;
    }

    return true;
}

bool AsyncReadFile::xwaitpid(int &pid, QStringList *errors, bool *pgot_pid_change)
{
    *pgot_pid_change = false;

    int status;
    const pid_t p = ::waitpid(pid, &status, WNOHANG);
    const int waitpid_errno = errno;

    if((p == 0) || (p == (-1) && waitpid_errno == EAGAIN)) {
        // ASFMYDBG("waitpid(PID=%d): still running", pid);
        return true;
    }

    if(p == (-1)) {
        ASFMYDBG("waitpid(PID=%d) = -1: %s", pid, strerror(waitpid_errno));
        force_kill_child("due to waitpid error");
        errors->append(make_latin1_errmsg(waitpid_errno, "waitpid error"));
        return false;
    }

    if(waitpid_errno && p != pid) {
        ASFMYDBG("waitpid(PID=%d) = %d: %s", pid, p, strerror(waitpid_errno));
    }
    else {
        ASFMYDBG("waitpid(PID=%d) = %d", pid, p);
    }

    if(p != pid) {
        force_kill_child("due to weird waitpid return %d", p);
        errors->append(make_latin1_errmsg(waitpid_errno, "waitpid weird return %d", p));
        return false;
    }

    *pgot_pid_change = true;


    // p == pid, finished
    pid = 0;


    if(WIFEXITED(status)) {
        const int ex = WEXITSTATUS(status);

        if(ex == 1) {
            errors->append(QStringLiteral("exit value 1"));
        }
        else if(ex == 0) {
            ASFMYDBG("success");
        }
        else {
            errors->append(QStringLiteral("exit value ") + QString::number(ex) + QStringLiteral("but I only expected 0 or 1"));
        }
    }

    else if(WIFSIGNALED(status)) {
        const int sig = WTERMSIG(status);
        errors->append(QStringLiteral("killed by signal ") + err_xbin_2_local_qstring(strsignal(sig)));
    }
    else if(errors->isEmpty()) {
        errors->append(QStringLiteral("did not properly exit"));
    }


    return false;
}

bool AsyncReadFile::iter(QStringList *errors, QByteArray *acc_contents, bool *p_made_progress)
{
    *p_made_progress = true;

    if(m_ofn.isEmpty()) {
        errors->append(QStringLiteral("iter() called but start() not called before"));
        return false;
    }

    // ******** ITER 1

    if(m_fn.isEmpty()) {
        m_fn = m_ofn;
        c_filename = ::strdup(m_fn.toLocal8Bit().constData());

        if(c_filename == NULL) {
            errors->append(QStringLiteral("out of memory"));
            return false;
        }

        return true;
    }

    // ******** ITER 2

    if(!m_done_pipefork) {

        int fds_msg[2] = { -1, -1 };
        int fds_content[2] = { -1, -1 };

        if(!xpipe2(fds_msg, "error message", errors)) {
            return false;
        }

        if(m_getc) {
            if(!xpipe2(fds_content, "content", errors)) {
                return false;
            }
        }

        m_pid = ::fork();

        if(m_pid < 0) {
            errors->append(make_latin1_errmsg(errno, "could not fork"));
            return false;
        }

        if(m_pid == 0) {
            char c_filename_short[20 + 1] = "";
            {
                const size_t l = strlen(c_filename);
                const size_t start = (l > 20 ? l - 20 : 0);
                strncpy(c_filename_short, c_filename + start, 20);
            }

            if(::close(fds_msg[PIPE_READ])) {
                CLDMYDBG("close(error message FD=%d): %s", fds_msg[PIPE_READ], strerror(errno));
            }
            else {
                CLDMYDBG("close(error message FD=%d)", fds_msg[PIPE_READ]);
            }

            fds_msg[PIPE_READ] = (-1);

            if(m_getc) {


                if(::close(fds_content[PIPE_READ])) {
                    CLDMYDBG("close(content FD=%d): %s", fds_content[PIPE_READ], strerror(errno));
                }
                else {
                    CLDMYDBG("close(content FD=%d)", fds_content[PIPE_READ]);
                }


                fds_content[PIPE_READ] = (-1);
            }

            FILE *msgfd_h = ::fdopen(fds_msg[PIPE_WRITE], "w");

            if(msgfd_h == NULL) {
                CLDMYDBG("could not fdopen(error message pipe FD=%d): %s", fds_msg[PIPE_WRITE], strerror(errno));
                char msg[] = "could not convert pipe to FILE";
                ssize_t byteswritten = ::write(fds_msg[PIPE_WRITE], msg, sizeof(msg));
                (void) byteswritten;
                ::_exit(1);
            }

            // pipe write, file read
            child_read_file(c_filename, c_filename_short, msgfd_h, fds_content[PIPE_WRITE], m_maxreadsize, pipe_bufsize, fileread_bufsize, CLF_DEBUG_enabled);
            // should never return
        }

        ASFMYDBG("spawned kid PID=%d", m_pid);

        (void)xclose(fds_msg[PIPE_WRITE], "error message");

        if(m_getc) {
            (void)xclose(fds_content[PIPE_WRITE], "content");
        }

        m_msgfd = fds_msg[PIPE_READ];
        m_contentfd = fds_content[PIPE_READ];

        m_done_pipefork = true;
        return true;
    }

    // ******** ITER 3...

    if(!m_done) {
        bool got_errmsg_data = false;

        if(m_msgfd >= 0) {

            if(m_errmsg_buffer == NULL) {
                m_errmsg_buffer = (char *)::malloc(errmsg_bufsize);

                if(m_errmsg_buffer == NULL) {
                    force_kill_child("no memory");
                    errors->append(make_latin1_errmsg(0, "no memory"));
                    return false;
                }
            }

            if(!xread(m_msgfd, "error message", m_errmsg_buffer, errmsg_bufsize, errors, &m_acc_err, &got_errmsg_data)) {
                return false;
            }

        }

        bool got_content_data = false;

        if(m_contentfd >= 0 && m_getc) {

            if(m_content_buffer_pipe_from_child == NULL) {
                m_content_buffer_pipe_from_child = (char *)::malloc(pipe_bufsize);

                if(m_content_buffer_pipe_from_child == NULL) {
                    force_kill_child("no memory");
                    errors->append(make_latin1_errmsg(0, "no memory"));
                    return false;
                }
            }

            if(!xread(m_contentfd, "content", m_content_buffer_pipe_from_child, pipe_bufsize, errors, acc_contents, &got_content_data)) {
                return false;
            }

        }

        *p_made_progress = (got_errmsg_data || got_content_data);

        // we only do this if there is a chance it will be interesting
        if((m_msgfd < 0 && m_contentfd < 0) || (!*p_made_progress)) {

            bool got_pid_change = false;

            if(!xwaitpid(m_pid, errors, &got_pid_change)) {
                m_done = true;
                *p_made_progress  = true;

                if(!m_acc_err.isEmpty()) {
                    errors->append(err_xbin_2_local_qstring(m_acc_err));
                    m_acc_err.clear();
                }

                return false;
            }

            *p_made_progress = (got_errmsg_data || got_content_data || got_pid_change);
        }

        return true;

    }

    // ******** ITER LAST

    if(m_done) {
        *p_made_progress = false;
        return false;
    }

    PROGRAMMERERROR("WTF");

}

void AsyncReadFile::finish()
{
    if(!m_ofn.isEmpty()) {
        ASFMYDBG("finish");
    }

    if(c_filename != NULL) {
        ::free(c_filename);
        c_filename = NULL;
    }

    (void)xclose(m_msgfd, "error message");

    (void)xclose(m_contentfd, "content");

    force_kill_child("still alive");

    if(m_errmsg_buffer != NULL) {
        ::free(m_errmsg_buffer);
        m_errmsg_buffer = NULL;
    }

    if(m_content_buffer_pipe_from_child != NULL) {
        ::free(m_content_buffer_pipe_from_child);
        m_content_buffer_pipe_from_child = NULL;
    }

    m_fn.clear();
    m_ofn.clear();
    m_acc_err.clear();
}

AsyncReadFile::~AsyncReadFile()
{
    finish();
}

void async_slurp_file(const QString &url, QStringList &errors, QByteArray &contents, const qint64 wholetimeout_msec, const qint64 sleep_usec)
{
    MYDBG("async_slurp_file: %s, fail after %lu msec", qPrintable(url), (unsigned long)wholetimeout_msec);

    AsyncReadFile ASF(url, true, -1);

    QElapsedTimer timer;
    timer.start();

    bool made_progress;

    while(ASF.iter(&errors, &contents, &made_progress)) {

        const qint64 timepassed_msec = timer.elapsed();

        if(timepassed_msec > wholetimeout_msec) {
            errors.append(QStringLiteral("took too long"));
            MYDBG("async_slurp_file: FAILURE %s", qPrintable(errors.join(QLatin1String("; "))));
            return;
        }

        if(!made_progress) {
            const qint64 time_still_allowed_msec = wholetimeout_msec - timepassed_msec;
            const qint64 timetosleep_usec = qMin(sleep_usec, 1000 * time_still_allowed_msec);
            ::usleep(timetosleep_usec);
        }

        QCoreApplication::processEvents();

    }

    if(errors.isEmpty()) {
        MYDBG("async_slurp_file: success");
    }
    else {
        MYDBG("async_slurp_file: FAILURE %s", qPrintable(errors.join(QLatin1String("; "))));
    }

}

QString try_load_start_of_file(const QString &url, const off_t maxreadsize, const qint64 wholetimeout_msec, const qint64 sleep_usec)
{

    MYDBG("try_load_start_of_file: %s, read first %lu bytes, fail after %lu msec", qPrintable(url), (unsigned long)maxreadsize, (unsigned long)wholetimeout_msec);

    QStringList errors;

    AsyncReadFile ASF(url, false, maxreadsize);

    QElapsedTimer timer;
    timer.start();

    bool made_progress;

    while(ASF.iter(&errors, NULL, &made_progress)) {

        const qint64 timepassed_msec = timer.elapsed();

        if(timepassed_msec > wholetimeout_msec) {
            errors.append(QStringLiteral("took too long"));
            const QString ret = errors.join(QStringLiteral("; "));
            MYDBG("try_load_start_of_file: FAILURE %s", qPrintable(ret));
            return ret;
        }

        if(!made_progress) {
            const qint64 time_still_allowed_msec = wholetimeout_msec - timepassed_msec;
            const qint64 timetosleep_usec = qMin(sleep_usec, 1000 * time_still_allowed_msec);
            ::usleep(timetosleep_usec);
        }

        QCoreApplication::processEvents();

    }

    const QString ret = errors.join(QStringLiteral("; "));

    if(ret.isEmpty()) {
        MYDBG("try_load_start_of_file: success");
    }
    else {
        MYDBG("try_load_start_of_file: FAILURE %s", qPrintable(ret));
    }

    return ret;

}
