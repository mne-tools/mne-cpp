//=============================================================================================================
/**
* @file     sourcelab.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the SourceLab class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelab.h"

#include "FormFiles/sourcelabsetupwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtConcurrent>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SourceLabPlugin;
using namespace FIFFLIB;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLab::SourceLab()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
, m_bFinishedClustering(false)
, m_qFileFwdSolution("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_sAtlasDir("./MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir("./MNE-sample-data/subjects/sample/surf")
, m_iNumAverages(10)
, m_bSingleTrial(false)
, m_iStimChan(0)
, m_iDownSample(4)
{

}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> SourceLab::clone() const
{
    QSharedPointer<SourceLab> pSourceLabClone(new SourceLab());
    return pSourceLabClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void SourceLab::init()
{
    // Inits
    m_pFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_qFileFwdSolution));
    m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));
    m_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet(m_sSurfaceDir+"/lh.white", m_sSurfaceDir+"/rh.white"));

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pSourceLabBuffer.isNull())
        m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr();

    // Input
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "SourceLabIn", "SourceLab input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &SourceLab::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTSEOutput = PluginOutputData<RealTimeSourceEstimate>::create(this, "SourceLabOut", "SourceLab output data");
    m_outputConnectors.append(m_pRTSEOutput);
    m_pRTSEOutput->data()->setName("Real-Time Source Estimate");
    m_pRTSEOutput->data()->setAnnotSet(m_pAnnotationSet);
    m_pRTSEOutput->data()->setSurfSet(m_pSurfaceSet);
    m_pRTSEOutput->data()->setSamplingRate(600/m_iDownSample);

    // start clustering
    QFuture<void> future = QtConcurrent::run(this, &SourceLab::doClustering);

}


//*************************************************************************************************************

void SourceLab::doClustering()
{
    emit clusteringStarted();
    m_bFinishedClustering = false;
    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(*m_pAnnotationSet.data(), 40)));

    finishedClustering();
}


//*************************************************************************************************************

void SourceLab::finishedClustering()
{
    m_pRTSEOutput->data()->setSrc(m_pClusteredFwd->src);

    m_bFinishedClustering = true;
    emit clusteringFinished();
}


//*************************************************************************************************************

bool SourceLab::start()
{
    if(m_bFinishedClustering)
    {
        QThread::start();
        return true;
    }
    else
        return false;
}


//*************************************************************************************************************

bool SourceLab::stop()
{
    m_bIsRunning = false;

    if(m_bProcessData) // Only clear if buffers have been initialised
    {
        m_pSourceLabBuffer->releaseFromPop();
        m_pSourceLabBuffer->releaseFromPush();
    }

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    if(m_pRtCov->isRunning())
        m_pRtCov->stop();

    if(m_pRtInvOp->isRunning())
        m_pRtInvOp->stop();

    if(m_pSourceLabBuffer)
        m_pSourceLabBuffer->clear();

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType SourceLab::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString SourceLab::getName() const
{
    return "SourceLab";
}


//*************************************************************************************************************

QWidget* SourceLab::setupWidget()
{
    SourceLabSetupWidget* setupWidget = new SourceLabSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    if(!m_bFinishedClustering)
        setupWidget->setClusteringState();

    connect(this, &SourceLab::clusteringStarted, setupWidget, &SourceLabSetupWidget::setClusteringState);
    connect(this, &SourceLab::clusteringFinished, setupWidget, &SourceLabSetupWidget::setSetupState);


    return setupWidget;
}


//*************************************************************************************************************

void SourceLab::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    //MEG
    if(pRTMSA && m_bReceiveData)
    {
        //Check if buffer initialized
        if(!m_pSourceLabBuffer)
            m_pSourceLabBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information
        if(!m_pFiffInfo)
            m_pFiffInfo = pRTMSA->getFiffInfo();

        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

            for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

            m_pSourceLabBuffer->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void SourceLab::appendEvoked(FiffEvoked::SPtr p_pEvoked)
{
    if(p_pEvoked->comment == QString("Stim %1").arg(m_iStimChan))
    {
        std::cout << p_pEvoked->comment.toLatin1().constData() << " append" << std::endl;

        mutex.lock();
        m_qVecEvokedData.push_back(p_pEvoked);
        mutex.unlock();
    }
}


//*************************************************************************************************************

void SourceLab::updateFiffCov(FiffCov::SPtr p_pFiffCov)
{
    m_pFiffCov = p_pFiffCov;

    if(m_pRtInvOp)
        m_pRtInvOp->appendNoiseCov(m_pFiffCov);
}


//*************************************************************************************************************

void SourceLab::updateInvOp(MNEInverseOperator::SPtr p_pInvOp)
{
    m_pInvOp = p_pInvOp;

    double snr = 3.0;
    double lambda2 = 1.0 / pow(snr, 2); //ToDO estimate lambda using covariance

    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

    mutex.lock();
    m_pMinimumNorm = MinimumNorm::SPtr(new MinimumNorm(*m_pInvOp.data(), lambda2, method));
    //
    //   Set up the inverse according to the parameters
    //
    m_pMinimumNorm->doInverseSetup(m_iNumAverages,false);
    mutex.unlock();
}


//*************************************************************************************************************

void SourceLab::run()
{
    m_bIsRunning = true;

    //
    // Cluster forward solution;
    //
//    qDebug() << "Start Clustering";
//    QFuture<MNEForwardSolution> future = QtConcurrent::run(this->m_pFwd.data(), &MNEForwardSolution::cluster_forward_solution, m_annotationSet, 40);
//    qDebug() << "Run Clustering";
//    future.waitForFinished();
//    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(future.result()));


    //Do this already in init
//    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(*m_pAnnotationSet.data(), 40)));

//    m_pRTSEOutput->data()->setSrc(m_pClusteredFwd->src);

    //
    // start receiving data
    //
    m_bReceiveData = true;

    //
    // Read Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    //
    // Init Real-Time Covariance estimator
    //
    m_pRtCov = RtCov::SPtr(new RtCov(5000, m_pFiffInfo));
    connect(m_pRtCov.data(), &RtCov::covCalculated, this, &SourceLab::updateFiffCov);

    //
    // Init Real-Time inverse estimator
    //
    m_pRtInvOp = RtInvOp::SPtr(new RtInvOp(m_pFiffInfo, m_pClusteredFwd));
    connect(m_pRtInvOp.data(), &RtInvOp::invOperatorCalculated, this, &SourceLab::updateInvOp);

    if(!m_bSingleTrial)
    {
        //
        // Init Real-Time average
        //
        m_pRtAve = RtAve::SPtr(new RtAve(m_iNumAverages, 750, 750, m_pFiffInfo));
        connect(m_pRtAve.data(), &RtAve::evokedStim, this, &SourceLab::appendEvoked);
    }

    //
    // Start the rt helpers
    //
    m_pRtCov->start();
    m_pRtInvOp->start();
    if(!m_bSingleTrial)
        m_pRtAve->start();

    //
    // start processing data
    //
    m_bProcessData = true;

    qint32 skip_count = 0;

    while(m_bIsRunning)
    {
        qint32 nrows = m_pSourceLabBuffer->rows();

        if(nrows > 0) // check if init
        {
            /* Dispatch the inputs */
            MatrixXd t_mat = m_pSourceLabBuffer->pop();

            //Add to covariance estimation
            m_pRtCov->append(t_mat);

            if(m_bSingleTrial)
            {
                //Continous Data
                mutex.lock();
                if(m_pMinimumNorm && t_mat.cols() > 0)
                {
                    //
                    // calculate the inverse
                    //
                    MNESourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_mat, 0, 1/m_pFiffInfo->sfreq);

                    std::cout << "Source Estimated" << std::endl;
                }
                mutex.unlock();
            }
            else
            {
                //Average Data
                m_pRtAve->append(t_mat);

                mutex.lock();
                if(m_pMinimumNorm && m_qVecEvokedData.size() > 0 && skip_count > 2)
                {
                    FiffEvoked t_fiffEvoked = *m_qVecEvokedData[0].data();

                    float tmin = ((float)t_fiffEvoked.first) / t_fiffEvoked.info.sfreq;
                    float tstep = 1/t_fiffEvoked.info.sfreq;

                    MNESourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_fiffEvoked.data, tmin, tstep);

                    std::cout << "SourceEstimated:\n" << std::endl;
    //                std::cout << "SourceEstimated:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;

                    //emit source estimates sample wise
                    for(qint32 i = 0; i < sourceEstimate.data.cols(); i += m_iDownSample)
                        m_pRTSEOutput->data()->setValue(sourceEstimate.data.col(i));

                    m_qVecEvokedData.pop_front();

                    skip_count = 0;
                }
                mutex.unlock();

                ++skip_count;
            }
        }
    }
}
