#ifndef ASYNCKILLPROC_H
#define ASYNCKILLPROC_H

#include <sys/types.h>

void async_kill_process(const pid_t &in_pid, char const *const in_kreason, char const *const in_context);

#endif // ASYNCKILLPROC_H
