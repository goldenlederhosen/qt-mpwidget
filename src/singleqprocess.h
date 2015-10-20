#ifndef SINGLEQPROCESS_H
#define SINGLEQPROCESS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QProcess>
#include <QStringList>

#include "qprocess_meta.h"
#include "deathsigprocess.h"

class SingleQProcess : public QObject
{
    Q_OBJECT
public:
    typedef QObject super;
private:
    DeathSigProcess *proc;
    QByteArray accout;
    QByteArray next;
    QString curr_exe;
    QStringList curr_args;
    void close();
private:
    // forbid
    SingleQProcess();
    SingleQProcess(const SingleQProcess &);
    SingleQProcess &operator=(const SingleQProcess &in);
public:
    explicit SingleQProcess(QObject *parent, const QString &in_oname);
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

protected:
    virtual bool event(QEvent *event);

};


#endif // SINGLEQPROCESS_H
