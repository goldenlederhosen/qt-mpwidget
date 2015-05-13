#ifndef ASYNCREADFILE_H
#define ASYNCREADFILE_H

#include <QString>
#include <QByteArray>
#include <QStringList>

class AsyncReadFile {
private:
    QString m_ofn;
    QString m_fn;
    bool m_getc;
    off_t m_maxreadsize;
    char *c_filename;
    QByteArray m_acc_err;

    bool m_done_pipefork;
    int m_msgfd;
    int m_contentfd;
    pid_t m_pid;

    bool m_done;
    size_t errmsg_bufsize;
    size_t fileread_bufsize;
    size_t pipe_bufsize;
    char *m_errmsg_buffer;
    char *m_content_buffer_pipe_from_child;

private:

    QString make_latin1_errmsg(int errno_copy, char const *const fmt, ...) const;
    void force_kill_child(char const *const fmt, ...);
    bool xpipe2(int *fds, char const *const desc, QStringList *errors);
    bool xclose(int &fd, char const *const desc);
    bool xread(int &fd, char const *const desc, char *buffer, size_t bufsize, QStringList *errors, QByteArray *accumulator, bool *got_bytes);
    bool xwaitpid(int &pid, QStringList *errors, bool *pgot_pid_change);

public:
    AsyncReadFile(const QString &in_fn, bool in_getc, off_t in_maxreadsize):
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
        , m_content_buffer_pipe_from_child(NULL) {
    }
    ~AsyncReadFile();
    bool iter(QStringList *errors, QByteArray *contents, bool *p_made_progress);
    void finish();
};

void async_slurp_file(const QString &url, QStringList &errors, QByteArray &contents, const qint64 wholetimeout_msec, const qint64 sleep_usec);
QString try_load_start_of_file(const QString &url, const off_t maxreadsize, const qint64 wholetimeout_msec, const qint64 sleep_usec);

#endif // ASYNCREADFILE_H
