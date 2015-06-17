#ifndef MOVIES_SYSTEM_H
#define MOVIES_SYSTEM_H

#include <QString>

bool find_dvddev(QString &retdev, QString &title, QString &body, QString &technical);

void add_exe_dir_to_PATH(char const *argv0);

void allow_core();

#endif // MOVIES_SYSTEM_H
