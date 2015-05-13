#ifndef SINGLEQPROCESS_H
#define SINGLEQPROCESS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QProcess>
#include <QStringList>

#include "qprocess_meta.h"

class SingleQProcess : public QObject {
    Q_OBJECT
private:
    QProcess *proc;
    QByteArray accout;
    QByteArray next;
    QString curr_exe;
    QStringList curr_args;
    void close();
public:
    explicit SingleQProcess(QObject *parent);
    virtual ~SingleQProcess();

    bool start(const QString &exe, const QStringList &args, QString &error);
    QString output_till_now() const;
    bool waitForFinished(int msecs);

signals:

    void sig_finished(bool success, const QString &errstring, const QString &output, const QString &exe, const QStringList &args);

public slots:

    // from the QProcess
    void slot_child_has_finished(int, QProcess::ExitStatus);
    void slot_accumulate_all_out();
};


#endif // SINGLEQPROCESS_H
