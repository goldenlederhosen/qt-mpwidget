#ifndef SINGLEQPROCESSSINGLESHOT_H
#define SINGLEQPROCESSSINGLESHOT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "singleqprocess.h"
#include "util.h"

class SingleQProcessSingleshot : public QObject
{
    Q_OBJECT

public:
    typedef QObject super;

private:
    SingleQProcess *prog;

    bool started;
    bool finished;

    bool m_success;
    QString m_errstr;
    QString m_output;
    QString m_exe;
    QStringList m_args;

    void check_is_finished() const
    {
        if(!started) {
            PROGRAMMERERROR("trying to get results, but start not called?");
        }

        if(!finished) {
            PROGRAMMERERROR("trying to get results, but finished signal not received?");
        }
    }

    bool start(const QString &exe, const QStringList &args, QString &error);
    bool wait(int msecs);

private:
    // forbid
    SingleQProcessSingleshot();
    SingleQProcessSingleshot(const SingleQProcessSingleshot &);
    SingleQProcessSingleshot &operator=(const SingleQProcessSingleshot &in);

public:
    explicit SingleQProcessSingleshot(QObject *parent, char const *const in_oname);

    virtual ~SingleQProcessSingleshot();

    bool run(const QString &exe, const QStringList &args, int waitmsecs, QString &error);

    bool get_success() const
    {
        check_is_finished();
        return m_success;
    }
    const QString &get_errstr() const
    {
        check_is_finished();
        return m_errstr;
    }
    const QString &get_output() const
    {
        check_is_finished();
        return m_output;
    }
    const QString &get_exe() const
    {
        check_is_finished();
        return m_exe;
    }
    const QStringList &get_args() const
    {
        check_is_finished();
        return m_args;
    }

public slots:

    // from the SingleQProcess
    void slot_prog_has_finished(bool success, const QString &errstr, const QString &output, const QString &exe, const QStringList &args);

protected:
    virtual bool event(QEvent *event);

};

#endif // SINGLEQPROCESSSINGLESHOT_H
