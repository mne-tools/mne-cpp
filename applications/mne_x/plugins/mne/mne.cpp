//=============================================================================================================
/**
* @file     mne.cpp
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
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the MNE class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"

#include "FormFiles/mnesetupwidget.h"


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

using namespace MNEPlugin;
using namespace FIFFLIB;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNE::MNE()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
, m_bFinishedClustering(false)
, m_qFileFwdSolution("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_sAtlasDir("./MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir("./MNE-sample-data/subjects/sample/surf")
, m_iNumAverages(10)
, m_iDownSample(4)
{

}


//*************************************************************************************************************

MNE::~MNE()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> MNE::clone() const
{
    QSharedPointer<MNE> pMNEClone(new MNE());
    return pMNEClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void MNE::init()
{
    // Inits
    m_pFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_qFileFwdSolution));
    m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));
    m_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet(m_sSurfaceDir+"/lh.white", m_sSurfaceDir+"/rh.white"));

    // Input
    m_pRTEInput = PluginInputData<RealTimeEvoked>::create(this, "MNE RTE In", "MNE real-time evoked input data");
    connect(m_pRTEInput.data(), &PluginInputConnector::notify, this, &MNE::updateRTE, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTEInput);

    m_pRTCInput = PluginInputData<RealTimeCov>::create(this, "MNE RTC In", "MNE real-time covariance input data");
    connect(m_pRTCInput.data(), &PluginInputConnector::notify, this, &MNE::updateRTC, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTCInput);

    // Output
    m_pRTSEOutput = PluginOutputData<RealTimeSourceEstimate>::create(this, "MNEOut", "MNE output data");
    m_outputConnectors.append(m_pRTSEOutput);
    m_pRTSEOutput->data()->setName("Real-Time Source Estimate");
    m_pRTSEOutput->data()->setAnnotSet(m_pAnnotationSet);
    m_pRTSEOutput->data()->setSurfSet(m_pSurfaceSet);

    // start clustering
    QFuture<void> future = QtConcurrent::run(this, &MNE::doClustering);

}


//*************************************************************************************************************

void MNE::calcFiffInfo()
{
    if(m_qListCovChNames.size() > 0 && m_pFiffInfoEvoked && m_pFiffInfoForward)
    {
        qDebug() << "Fiff Infos available";

        QStringList tmp_pick_ch_names;
        foreach (const QString &ch, m_pFiffInfoForward->ch_names)
        {
            if(m_pFiffInfoEvoked->ch_names.contains(ch))
                tmp_pick_ch_names << ch;
        }

        m_qListPickChannels.clear();

        foreach (const QString &ch, tmp_pick_ch_names)
        {
            if(m_qListCovChNames.contains(ch))
                m_qListPickChannels << ch;
        }

        RowVectorXi sel = m_pFiffInfoEvoked->pick_channels(m_qListPickChannels);

        m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(m_pFiffInfoEvoked->pick_info(sel)));
    }

}


//*************************************************************************************************************

void MNE::doClustering()
{
    emit clusteringStarted();
    m_bFinishedClustering = false;
    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(*m_pAnnotationSet.data(), 40)));

    finishedClustering();
}


//*************************************************************************************************************

void MNE::finishedClustering()
{
    m_bFinishedClustering = true;

    m_pFiffInfoForward = QSharedPointer<FiffInfoBase>(new FiffInfoBase(m_pClusteredFwd->info));

    emit clusteringFinished();
}


//*************************************************************************************************************

bool MNE::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(m_bFinishedClustering)
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
        return false;
}


//*************************************************************************************************************

bool MNE::stop()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = false;

    if(m_bProcessData) // Only clear if buffers have been initialised
    {
        m_qVecFiffEvoked.clear();
        m_qVecFiffCov.clear();
    }

    m_qListCovChNames.clear();

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    if(m_pRtInvOp->isRunning())
        m_pRtInvOp->stop();

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType MNE::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString MNE::getName() const
{
    return "MNE";
}


//*************************************************************************************************************

QWidget* MNE::setupWidget()
{
    MNESetupWidget* setupWidget = new MNESetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    if(!m_bFinishedClustering)
        setupWidget->setClusteringState();

    connect(this, &MNE::clusteringStarted, setupWidget, &MNESetupWidget::setClusteringState);
    connect(this, &MNE::clusteringFinished, setupWidget, &MNESetupWidget::setSetupState);

    return setupWidget;
}


//*************************************************************************************************************

void MNE::updateRTC(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeCov> pRTC = pMeasurement.dynamicCast<RealTimeCov>();

    //MEG
    if(pRTC && m_bReceiveData)
    {
        //Fiff Information of the covariance
        if(m_qListCovChNames.size() != pRTC->getValue()->names.size())
            m_qListCovChNames = pRTC->getValue()->names;

        if(m_bProcessData)
        {
            mutex.lock();
            m_qVecFiffCov.push_back(pRTC->getValue()->pick_channels(m_qListPickChannels));
            mutex.unlock();
        }
    }
}



//*************************************************************************************************************

void MNE::updateRTE(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeEvoked> pRTE = pMeasurement.dynamicCast<RealTimeEvoked>();

    //MEG
    if(pRTE && m_bReceiveData)
    {
        //Fiff Information of the evoked
        if(!m_pFiffInfoEvoked)
            m_pFiffInfoEvoked = QSharedPointer<FiffInfo>(new FiffInfo(pRTE->getValue()->info));

        if(m_bProcessData)
        {
            mutex.lock();
            m_qVecFiffEvoked.push_back(pRTE->getValue()->pick_channels(m_qListPickChannels));
            mutex.unlock();
        }
    }
}


//*************************************************************************************************************

void MNE::updateInvOp(MNEInverseOperator::SPtr p_pInvOp)
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

void MNE::run()
{
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
    {
        calcFiffInfo();
        msleep(10);// Wait for fiff Info
    }

    //
    // Init Real-Time inverse estimator
    //
    m_pRtInvOp = RtInvOp::SPtr(new RtInvOp(m_pFiffInfo, m_pClusteredFwd));
    connect(m_pRtInvOp.data(), &RtInvOp::invOperatorCalculated, this, &MNE::updateInvOp);

    m_pMinimumNorm.reset();

    //
    // Start the rt helpers
    //
    m_pRtInvOp->start();

    //
    // start processing data
    //
    m_bProcessData = true;

    qint32 skip_count = 0;

    while(m_bIsRunning)
    {
        if(m_qVecFiffCov.size() > 0)
        {
            mutex.lock();
            qDebug() << "m_qVecFiffCov" << m_qVecFiffCov.size();
            m_pRtInvOp->appendNoiseCov(m_qVecFiffCov[0]);//DEBUG THIS
            m_qVecFiffCov.pop_front();
            mutex.unlock();
        }

        if(m_qVecFiffEvoked.size() > 0)
        {
            //DEBUG THIS
            if(m_pMinimumNorm)
            {
                mutex.lock();
                FiffEvoked t_fiffEvoked = m_qVecFiffEvoked[0];
                m_qVecFiffEvoked.pop_front();
                mutex.unlock();

                qDebug() << "source estimate replacement";
                float tmin = ((float)t_fiffEvoked.first) / t_fiffEvoked.info.sfreq;
                float tstep = 1/t_fiffEvoked.info.sfreq;

                MNESourceEstimate sourceEstimate = m_pMinimumNorm->calculateInverse(t_fiffEvoked.data, tmin, tstep);

                m_pRTSEOutput->data()->setValue(sourceEstimate);
            }
            else
            {
                mutex.lock();
                m_qVecFiffEvoked.pop_front();
                mutex.unlock();
            }
        }
    }
}
