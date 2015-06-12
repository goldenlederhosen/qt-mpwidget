#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

#include "asyncreadfile_child.h"
#include "util.h"

//#define DEBUG_CLF

#ifdef DEBUG_ALL
#define DEBUG_CLF
#endif

#ifdef DEBUG_CLF
#  define CLDMYDBG(msg, ...) fprintf(stderr, "CLF C  %s PID=%d " msg "\n", c_filename_short, getpid(), ##__VA_ARGS__)
#else
#  define CLDMYDBG(msg, ...)
#endif


const int sleep_if_read_not_ready_usec = 1000;
const int sleep_if_pipe_full_usec = 1000;
const int sleep_after_successful_write_usec = 0;

template<typename T>
static inline T MIN(const T a, const T b)
{
    if(a < b) {
        return a;
    }
    else {
        return b;
    }
}

static void childexit(bool succ, FILE *fh, char const *const c_filename_short)
{
    (void)c_filename_short;
    const int ex = succ ? 0 : 1;

    ::fflush(stdout);
    ::fflush(stderr);

    if(fh != NULL) {
        CLDMYDBG("closing FD=%d", fileno(fh));
        ::fflush(fh);
        ::fclose(fh);
    }

    CLDMYDBG("exit = %d", ex);
    ::_exit(ex);
}

static void childerrfatal(FILE *errmsg_write_fh, char const *const c_filename_short, char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "%s CLF C erroring with ", c_filename_short);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    if(vfprintf(errmsg_write_fh, fmt, ap) <= 0) {
        fprintf(stderr, "could not write error message to channel FD=%d: %s", fileno(errmsg_write_fh), strerror(errno));
    }

    va_end(ap);

    childexit(false, errmsg_write_fh, c_filename_short);
}

static void usleep_verboseonerr(unsigned long usecs, char const *const c_filename_short)
{
    (void)c_filename_short;


    if(usecs == 0) {
        return;
    }

    if(::usleep(usecs)) {
        CLDMYDBG("usleep(%lu)", usecs);
        return;
    }

    CLDMYDBG("usleep(%lu): %s", usecs, strerror(errno));
}

static void xclose(int &fd, FILE *errmsg_write_fh, char const *const c_filename_short)
{
    if(fd >= 0) {
        if(::close(fd)) {
            childerrfatal(errmsg_write_fh, c_filename_short, "could not close file FD=%d: %s", fd, strerror(errno));
        }
        else {
            CLDMYDBG("close(FD=%d)", fd);
        }

        fd = (-1);
    }
}

static off_t xfsize(int fd, FILE *errmsg_write_fh, char const *const c_filename_short)
{
    struct stat st;

    if(::fstat(fd, &st)) {
        childerrfatal(errmsg_write_fh, c_filename_short, "could not stat file FD=%d: %s", fd, strerror(errno));
    }

    const off_t size = st.st_size;
    return size;
}

static int xopen(char const *const c_filename, FILE *errmsg_write_fh, char const *const c_filename_short)
{
    int file_read_fd = ::open(c_filename, O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NDELAY);

    if(file_read_fd < 0) {
        childerrfatal(errmsg_write_fh, c_filename_short, "could not open: %s", strerror(errno));
    }

    CLDMYDBG("opened as FD=%d", file_read_fd);
    return file_read_fd;
}

void child_read_file(char const *const c_filename, char const *const c_filename_short, FILE *errmsg_write_fh,  int file_write_fd, const off_t maxreadsize, const off_t pipe_write_blocksize, const off_t file_read_blocksize)
{

    CLDMYDBG("hi from child. errmsg FD=%d, pipe FD=%d", fileno(errmsg_write_fh), file_write_fd);

    int file_read_fd = xopen(c_filename, errmsg_write_fh, c_filename_short);

    const off_t size = xfsize(file_read_fd, errmsg_write_fh, c_filename_short);

    const off_t toread = (maxreadsize < 0 ? size : (size > maxreadsize ? maxreadsize : size));

    if(maxreadsize < 0) {
        if(file_write_fd >= 0) {
            CLDMYDBG("read moviefile size: %lu bytes. Will try to read all of that, in %lu byte blocks, and transmit in %lu byte blocks", (unsigned long) size, (unsigned long) file_read_blocksize, (unsigned long) pipe_write_blocksize);
        }
        else {
            CLDMYDBG("read moviefile size: %lu bytes. Will try to read all of that, in %lu byte blocks", (unsigned long) size, (unsigned long) file_read_blocksize);
        }
    }
    else {
        if(file_write_fd >= 0) {
            CLDMYDBG("read moviefile size: %lu bytes. Will try to read first %lu bytes of that, in %lu byte blocks, and transmit in %lu byte blocks", (unsigned long) size, (long unsigned) toread, (unsigned long) file_read_blocksize, (unsigned long) pipe_write_blocksize);
        }
        else {
            CLDMYDBG("read moviefile size: %lu bytes. Will try to read first %lu bytes of that, in %lu byte blocks", (unsigned long) size, (long unsigned) toread, (unsigned long) file_read_blocksize);
        }
    }

    if(file_read_blocksize <= 0) {
        qFatal("file blocksize negative/0?");
    }

    if(file_write_fd >= 0) {
        if(pipe_write_blocksize <= 0) {
            qFatal("pipe blocksize negative/0?");
        }

        if(pipe_write_blocksize > file_read_blocksize) {
            qFatal("pipe blocksize needs to be smaller or equal to file blocksize");
        }

        {
            const int file_to_pipe = file_read_blocksize / pipe_write_blocksize;

            if(file_to_pipe * pipe_write_blocksize != file_read_blocksize) {
                qFatal("file blocksize needs to be a multiple of pipe blocksize");
            }
        }
    }

    char *file_buffer = (char *)::malloc(file_read_blocksize);

    if(file_buffer == NULL) {
        childerrfatal(errmsg_write_fh, c_filename_short, "no memory");
    }

    size_t readiters = 0;
    off_t pos = 0;

    while(true) {

        // read one block of data, advance nexttask on EOF
        if(pos >= toread) {
            CLDMYDBG("read everything. Read %lu blocks", readiters);
            break;
        }

        // wait for data to be ready
        {
            int nfds = file_read_fd + 1;
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(file_read_fd, &readfds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = sleep_if_read_not_ready_usec;
            int selectret = select(nfds, &readfds, NULL, NULL, &timeout);

            if(selectret < 0) {
                childerrfatal(errmsg_write_fh, c_filename_short, "error waiting for data: %s", strerror(errno));
            }

            if(selectret == 0) {
                CLDMYDBG("data is not available on file FD=%d", file_read_fd);
                //usleep_verboseonerr(sleep_if_read_not_ready_usec, c_filename_short);
                continue;
            }

            if(!FD_ISSET(file_read_fd, &readfds)) {
                childerrfatal(errmsg_write_fh, c_filename_short, "select returned but file FD is not set?");
            }

            CLDMYDBG("data is available on file FD=%d", file_read_fd);
        }

        ssize_t bytes_read = ::read(file_read_fd, file_buffer, file_read_blocksize);

        readiters++;

        if(bytes_read == -1) {
            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                childerrfatal(errmsg_write_fh, c_filename_short, "error reading: %s", strerror(errno));
            }

            // since we had a select before and the FD is blocking this should never happen
            CLDMYDBG("wait on next data from file FD=%d to become available", file_read_fd);
            usleep_verboseonerr(sleep_if_read_not_ready_usec, c_filename_short);
            continue;
        }

        if(bytes_read == 0) {
            // EOF
            if(pos < toread) {
                childerrfatal(errmsg_write_fh, c_filename_short, "got EOF before reading all of file?");
            }

            CLDMYDBG("EOF after %lu blocks", readiters);
            break;
        }

        CLDMYDBG("read %lu bytes from file FD=%d", (unsigned long)bytes_read, file_read_fd);

        if(file_write_fd >= 0) {

            size_t bytes_written = 0;

            while(bytes_written < (size_t)bytes_read) {
                const size_t bytes_to_write = bytes_read - bytes_written;
                const size_t bytes_to_write_at_once = MIN((size_t)bytes_to_write, (size_t)pipe_write_blocksize);

                // wait for pipe to be ready
                {
                    int nfds = file_write_fd + 1;
                    fd_set writefds;
                    FD_ZERO(&writefds);
                    FD_SET(file_write_fd, &writefds);
                    struct timeval timeout;
                    timeout.tv_sec = 0;
                    timeout.tv_usec = sleep_if_pipe_full_usec;
                    int selectret = select(nfds, NULL, &writefds, NULL, &timeout);

                    if(selectret < 0) {
                        childerrfatal(errmsg_write_fh, c_filename_short, "error waiting for pipe to drain: %s", strerror(errno));
                    }

                    if(selectret == 0) {
                        // since we did not give a timeout this should never happen....
                        CLDMYDBG("data can not be written to pipe FD=%d", file_write_fd);
                        //usleep_verboseonerr(sleep_if_pipe_full_usec, c_filename_short);
                        continue;
                    }

                    if(!FD_ISSET(file_write_fd, &writefds)) {
                        childerrfatal(errmsg_write_fh, c_filename_short, "select returned but file FD is not set?");
                    }

                    CLDMYDBG("data can be written to pipe FD=%d", file_write_fd);
                }

                CLDMYDBG("trying to write %lu bytes to FD=%d", (unsigned long)bytes_to_write_at_once, file_write_fd);
                const ssize_t bw = write(file_write_fd, file_buffer + bytes_written, bytes_to_write_at_once);

                if(bw < 0) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        // fine....
                        CLDMYDBG("FD=%d not ready, weird", file_write_fd);

                        // this should never happen since we used the select above
                        usleep_verboseonerr(sleep_if_pipe_full_usec, c_filename_short);
                        continue;
                    }

                    childerrfatal(errmsg_write_fh, c_filename_short, "error writing: %s", strerror(errno));
                }
                else if(bw == 0) {
                    CLDMYDBG("write(FD=%d) returned 0?", file_write_fd);
                    continue;
                }
                else {
                    bytes_written += bw;

                    // give other end time to react
                    usleep_verboseonerr(sleep_after_successful_write_usec, c_filename_short);
                }

            }

        }

        // okay, we got some bytes, carrying on...
        //CLDMYDBG("read %lu bytes from file", (unsigned long)bytes_read);
        pos += bytes_read;

    }

    xclose(file_read_fd, errmsg_write_fh, c_filename_short);
    xclose(file_write_fd, errmsg_write_fh, c_filename_short);

    CLDMYDBG("successfully read %lu bytes, exiting", (unsigned long) toread);
    childexit(true, errmsg_write_fh, c_filename_short);
}
