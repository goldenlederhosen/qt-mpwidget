#ifndef DEATHSIGPROCESS
#define DEATHSIGPROCESS

#include <QProcess>

class DeathSigProcess : public QProcess
{
    Q_OBJECT
public:
    typedef QProcess super;
private:
    void init(const QString &oName, QObject *parent);

    // forbid
    DeathSigProcess();
    DeathSigProcess(const DeathSigProcess &);
    DeathSigProcess &operator=(const DeathSigProcess &in);

public:
    DeathSigProcess(char const *const oName_latin1, QObject *parent);
    DeathSigProcess(const QString &oName, QObject *parent);
    virtual ~DeathSigProcess();
protected:
    virtual bool event(QEvent *event);
    virtual void setupChildProcess();
public slots:
    void slot_dbg_error(QProcess::ProcessError error);
    void slot_dbg_finished(int exitCode, QProcess::ExitStatus exitStatus);
    void slot_dbg_readyReadStandardError();
    void slot_dbg_readyReadStandardOutput();
    void slot_dbg_started();
    void slot_dbg_stateChanged(QProcess::ProcessState newState);
    void slot_dbg_aboutToClose();
    void slot_dbg_bytesWritten(qint64 bytes);
    void slot_dbg_readChannelFinished();
    void slot_dbg_readyRead();
};


#endif // DEATHSIGPROCESS
