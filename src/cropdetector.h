#ifndef CROPDETECTOR_H
#define CROPDETECTOR_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDateTime>

class CropDetector : public QObject {
    Q_OBJECT
private:
    QString mfn;
    QProcess proc;
    QDateTime start;
public:
    explicit CropDetector(QObject *parent, QString in_mfn);

signals:

    void sig_detected(bool, QString, QString);

public slots:

    // from the process
    void slot_pError(QProcess::ProcessError);
    void slot_pfinished(int, QProcess::ExitStatus);

};

#endif // CROPDETECTOR_H
