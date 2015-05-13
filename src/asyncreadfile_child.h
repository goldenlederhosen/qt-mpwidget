#ifndef ASYNCREADFILECHILD_H
#define ASYNCREADFILECHILD_H

#include <cstdio>

void child_read_file(char const *const c_filename, char const *const c_filename_short, FILE *errmsg_write_fh,  int file_write_fd, const off_t maxreadsize, const off_t pipe_write_blocksize, const off_t file_read_blocksize);

#endif // ASYNCREADFILECHILD_H
