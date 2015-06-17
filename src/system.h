#ifndef MOVIES_SYSTEM_H
#define MOVIES_SYSTEM_H

#include <QString>

bool find_dvddev(QString &retdev, QString &title, QString &body, QString &technical);

void add_exe_dir_to_PATH(char const *argv0);

void allow_core();

bool definitely_running_from_desktop();

QString compute_MP_VO();
QString compute_VDB_RUN(QString *reason);

#endif // MOVIES_SYSTEM_H
