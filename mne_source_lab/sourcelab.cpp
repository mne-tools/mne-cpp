
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelab.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../MNE/mne/mne.h"
#include "../MNE/fs/annotation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
//#include <QDebug>
//#include <QDir>
//#include <QPluginLoader>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;


SourceLab::SourceLab(QObject *parent)
: QThread(parent)
, m_pRtDataManager(NULL)
, m_pRawInfo(NULL)
, m_bIsRunning(false)
, m_pRawMatrixBuffer(NULL)
{
//    MNEForwardSolution* t_pFwd = NULL;

//    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";

//    QFile* t_pFile = new QFile(t_sFileName);

//    if(!MNEForwardSolution::read_forward_solution(t_pFile, t_pFwd))
//    {
//        delete t_pFile;
//        if(t_pFwd)
//            delete t_pFwd;
////        return -1;
//        return;
//    }

//    //Cluster the forward solution

//    MNEForwardSolution* t_pFwdClustered = NULL;

//    QString t_sLHAnnotFileName = "./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot";
//    Annotation* t_pLHAnnotation= new Annotation(t_sLHAnnotFileName);


//    QString t_sRHAnnotFileName = "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot";
//    Annotation* t_pRHAnnotation= new Annotation(t_sRHAnnotFileName);

//    t_pFwd->cluster_forward_solution(t_pFwdClustered, t_pLHAnnotation, t_pRHAnnotation, 40);

//    delete t_pFile;
//    delete t_pFwd;


    m_pRtDataManager = new RtDataManager(this);

    m_pRtDataManager->start();

}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    if(m_pRtDataManager)
        delete m_pRtDataManager;
}


//*************************************************************************************************************

bool SourceLab::start()
{
    // Start threads
    m_pRtDataManager->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool SourceLab::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void SourceLab::run()
{
    m_bIsRunning = true;

    quint32 count = 0;

    while(m_bIsRunning)
    {
        MatrixXf rawSegment = m_pRawMatrixBuffer->pop();
        qDebug() << "Received Buffer " << count;
        ++count;


        qint32 samples = rawSegment.cols();
        VectorXf mu = rawSegment.rowwise().sum().array() / (float)samples;

        MatrixXf noise_covariance = rawSegment * rawSegment.transpose();// noise_covariance == raw_covariance
        noise_covariance.array() -= samples * (mu * mu.transpose()).array();
        noise_covariance.array() /= (samples - 1);


        std::cout << "Noise Covariance:\n" << noise_covariance.block(0,0,10,10) << std::endl;

//        printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

//        emit remitRawBuffer(tmp);
    }

}
