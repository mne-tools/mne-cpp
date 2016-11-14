#ifndef MNELAUNCHCONTROL_H
#define MNELAUNCHCONTROL_H


#include <QObject>
#include <QProcess>
#include <QPointer>
#include <QList>

class MNELaunchControl : public QObject
{
    Q_OBJECT

    Q_PROPERTY ( bool sampleDataAvailable READ getSampleDataAvailable NOTIFY sampleDataAvailableChanged_Signal )

public:
    MNELaunchControl(QObject *parent = nullptr);

    virtual ~MNELaunchControl();

    Q_INVOKABLE void invokeScan();
    Q_INVOKABLE void invokeBrowse();
    Q_INVOKABLE void invokeAnalyze();

    void invokeApplication(const QString& application, const QStringList& arguments);

    bool getSampleDataAvailable() const;

signals:
    void sampleDataAvailableChanged_Signal();

private:
    QList<QPointer<QProcess>> m_ListProcesses;
    QStringList m_requiredSampleFiles;
};

#endif // MNELAUNCHCONTROL_H
