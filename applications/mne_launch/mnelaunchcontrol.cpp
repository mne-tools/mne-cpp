#include "mnelaunchcontrol.h"

#include <QDebug>
#include <QCoreApplication>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNELaunchControl::MNELaunchControl(QObject *parent)
: QObject(parent)
{
    m_requiredSampleFiles   <<  "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif"
                            <<  "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif"
                            <<  "/MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot"
                            <<  "/MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot";
}


//*************************************************************************************************************

MNELaunchControl::~MNELaunchControl()
{

}


//*************************************************************************************************************

void MNELaunchControl::invokeScan()
{
    invokeApplication("mne_scan",QStringList());
}


//*************************************************************************************************************

void MNELaunchControl::invokeBrowse()
{
    invokeApplication("mne_browse",QStringList());
}


//*************************************************************************************************************

void MNELaunchControl::invokeAnalyze()
{
    invokeApplication("mne_analyze",QStringList());
}


//*************************************************************************************************************

void MNELaunchControl::invokeApplication(const QString &application, const QStringList &arguments)
{
    QFile file(QCoreApplication::applicationDirPath()+ "/" + application);
#if defined(_WIN32) || defined(_WIN32_WCE)
    file.setFileName(file.fileName() + ".exe");
#endif

    if(file.exists()) {
        try {
            QPointer<QProcess> process( new QProcess );
            process->start(file.fileName(), arguments);
            m_ListProcesses.append(process);
        } catch (int e) {
            qWarning() << "Not able to start" << file.fileName() << ". Error:" << e;
        }
    }
    else {
        qWarning() << "Application" << file.fileName() << "does not exists.";
    }
}


//*************************************************************************************************************

bool MNELaunchControl::getSampleDataAvailable() const
{
    bool sampleFilesExist = true;

    for(QString fileName : m_requiredSampleFiles)
    {
        QFile file (QCoreApplication::applicationDirPath() + fileName);
        if ( !file.exists() ) {
            qWarning() << "Missing sample file" << file.fileName();
            sampleFilesExist = false;
        }
    }
    return sampleFilesExist;
}
