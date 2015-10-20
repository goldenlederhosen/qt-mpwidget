#ifndef CROPDETECTOR_H
#define CROPDETECTOR_H

#include <QObject>
#include <QString>
#include <QDateTime>

#include "deathsigprocess.h"

class CropDetector : public QObject
{
    Q_OBJECT
public:
    typedef QObject super;
private:
    QString mfn;
    QString fshort;
    DeathSigProcess proc;
    QDateTime start;

    // forbid
    CropDetector();
    CropDetector(const CropDetector &);
    CropDetector &operator=(const CropDetector &in);

public:
    explicit CropDetector(QObject *parent, QString in_mfn);
    virtual ~CropDetector() {}

signals:

    void sig_detected(bool, QString, QString);

public slots:

    // from the process
    void slot_pError(QProcess::ProcessError);
    void slot_pfinished(int, QProcess::ExitStatus);

protected:
    virtual bool event(QEvent *event);

};

#endif // CROPDETECTOR_H
