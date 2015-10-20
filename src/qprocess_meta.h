#ifndef QPROCESS_META_H
#define QPROCESS_META_H

#include <QProcess>
#include <QMetaType>

Q_DECLARE_METATYPE(QProcess::ExitStatus)
Q_DECLARE_METATYPE(QProcess::ProcessError)

char const *ProcessError_2_latin1str(QProcess::ProcessError e);

#endif // QPROCESS_META_H
