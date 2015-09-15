#include "remote_local.h"

#include <sys/vfs.h>            /* or <sys/statfs.h> */
#include <cerrno>
#include <cstring>
#include <QDebug>

#include "util.h"
#include "fstypemagics.h"

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY "REMLOC"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

enum class fs_remote_local {
    remote, local, unknown
};

static fs_remote_local
query_path_remote_local(char const *const path)
{

    if(path == NULL || path[0] == '\0') {
        PROGRAMMERERROR("path empty");
    }

    struct statfs buf;

    errno = 0;

    if(statfs(path, &buf)) {
        /* error in the statfs() call */
        MYDBG("could not statfs(%s): %s", path, strerror(errno));
        return fs_remote_local::unknown;
    }

    const long type = buf.f_type;

    /* remote_magic is a list of known-remote fs types */
    size_t i = 0;

    while(remote_magic[i] != 0) {
        if(type == remote_magic[i]) {
            MYDBG("query_path_remote_local(%s): definitely remote %s", path, remote_magic_name[i]);
            return fs_remote_local::remote;
        }

        i++;
    }

    /* local_magic is a list of known-local fs types */
    i = 0;

    while(local_magic[i] != 0) {
        if(type == local_magic[i]) {
            MYDBG("query_path_remote_local(%s): definitely local %s", path, local_magic_name[i]);
            return fs_remote_local::local;
        }

        i++;
    }

    MYDBG("could not determine whether filesystem on %s (f_type=%ld) is local or remote", path, type);
    return fs_remote_local::unknown;
}

bool
path_is_definitely_remote(char const *const path)
{
    return query_path_remote_local(path) == fs_remote_local::remote;
}

bool
path_is_definitely_local(char const *const path)
{
    return query_path_remote_local(path) == fs_remote_local::local;
}
