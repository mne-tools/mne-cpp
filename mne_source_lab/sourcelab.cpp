
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

#include <fs/annotation.h>


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
, m_pFwd(MNEForwardSolution::SPtr(new MNEForwardSolution()))
, m_bIsRunning(false)
, m_bIsRawBufferInit(false)
, m_pRawMatrixBuffer(NULL)
{
    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";

    QFile t_File(t_sFileName);

    if(!MNEForwardSolution::read_forward_solution(t_File, *m_pFwd))
        return;

    //Cluster the forward solution

    QString t_sLHAnnotFileName = "./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot";
    Annotation t_LHAnnotation(t_sLHAnnotFileName);


    QString t_sRHAnnotFileName = "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot";
    Annotation t_RHAnnotation(t_sRHAnnotFileName);

//    MNEForwardSolution t_FwdClustered;
//    m_pFwd->cluster_forward_solution(t_FwdClustered, t_LHAnnotation, t_RHAnnotation, 40);

    qRegisterMetaType<MatrixXf>("MatrixXf");
    qRegisterMetaType<MNEInverseOperator>("MNEInverseOperator");
    qRegisterMetaType<FiffCov>("FiffCov");

    m_pRtClient = new RtClient("127.0.0.1", this);
    connect(m_pRtClient, &RtClient::rawBufferReceived,
            this, &SourceLab::receiveRawBuffer);
    this->start();

    // ToDo --> rtclient emit FiffInfoAvailable signal and call a slot here
    //DIRTY HACK - following code has to be moved -> it waits until FiffInfo is available
    bool t_bFiffInfoAvailable = false;
    while(!t_bFiffInfoAvailable)
    {
        mutex.lock();
        if(m_pRtClient->getFiffInfo().ch_names.size() > 0)
            t_bFiffInfoAvailable = true;
        mutex.unlock();
        sleep(1.0);
    }

    m_fiffInfo = FiffInfo(m_pRtClient->getFiffInfo());

    m_pRtInv = new RtInv(m_fiffInfo, m_pFwd, this);

    m_pRtCov = new RtCov(m_fiffInfo, this);
    connect(m_pRtClient, &RtClient::rawBufferReceived,
            m_pRtCov, &RtCov::receiveDataSegment);
    m_pRtCov->start();

    connect(m_pRtCov, &RtCov::covCalculated,
            m_pRtInv, &RtInv::receiveNoiseCov);
    m_pRtInv->start();
}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    if(m_pRtClient)
        delete m_pRtClient;
    if(m_pRawMatrixBuffer)
        delete m_pRawMatrixBuffer;
}


//*************************************************************************************************************

bool SourceLab::start()
{
    // Start threads
    m_pRtClient->start();

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

void SourceLab::receiveRawBuffer(MatrixXf p_rawBuffer)
{
    if(!m_pRawMatrixBuffer)
    {
        mutex.lock();
        m_pRawMatrixBuffer = new RawMatrixBuffer(10, p_rawBuffer.rows(), p_rawBuffer.cols());
        m_bIsRawBufferInit = true;
        mutex.unlock();
    }

    m_pRawMatrixBuffer->push(&p_rawBuffer);
}


//*************************************************************************************************************

void SourceLab::run()
{
    m_bIsRunning = true;

    quint32 count = 0;

    while(m_bIsRunning)
    {
        if(m_bIsRawBufferInit && !m_pRtClient->getFiffInfo().isEmpty())
        {
            MatrixXf rawSegment = m_pRawMatrixBuffer->pop();
//            qDebug() << "Received Buffer " << count;
//            ++count;

//            qint32 samples = rawSegment.cols();
//            VectorXf mu = rawSegment.rowwise().sum().array() / (float)samples;

//            MatrixXf noise_covariance = rawSegment * rawSegment.transpose();// noise_covariance == raw_covariance
//            noise_covariance.array() -= samples * (mu * mu.transpose()).array();
//            noise_covariance.array() /= (samples - 1);

//            std::cout << "Noise Covariance:\n" << noise_covariance.block(0,0,10,10) << std::endl;

//            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

        }
    }
}
