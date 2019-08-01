//=============================================================================================================
/**
* @file     rapmusictoolbox.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2014
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
* @brief    Definition of the RapMusicToolbox class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rapmusictoolbox.h"

#include "FormFiles/rapmusictoolboxsetupwidget.h"


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

using namespace RapMusicToolboxPlugin;
using namespace FIFFLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RapMusicToolbox::RapMusicToolbox()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
, m_bFinishedClustering(false)
, m_qFileFwdSolution(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_sAtlasDir(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/surf")
, m_iNumAverages(10)
, m_iDownSample(4)
{

}


//*************************************************************************************************************

RapMusicToolbox::~RapMusicToolbox()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> RapMusicToolbox::clone() const
{
    QSharedPointer<RapMusicToolbox> pRapMusicToolboxClone(new RapMusicToolbox());
    return pRapMusicToolboxClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void RapMusicToolbox::init()
{
    // Inits
    m_pFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_qFileFwdSolution));
    m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));
    m_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet(m_sSurfaceDir+"/lh.white", m_sSurfaceDir+"/rh.white"));

    // Input
    m_pRTEInput = PluginInputData<RealTimeEvokedSet>::create(this, "RapMusic Toolbox RTE In", "RapMusic Toolbox real-time evoked input data");
    connect(m_pRTEInput.data(), &PluginInputConnector::notify, this, &RapMusicToolbox::updateRTE, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTEInput);

    // Output
    m_pRTSEOutput = PluginOutputData<RealTimeSourceEstimate>::create(this, "MNEOut", "RapMusic Toolbox output data");
    m_outputConnectors.append(m_pRTSEOutput);
    m_pRTSEOutput->data()->setName("Real-Time Source Estimate");
    m_pRTSEOutput->data()->setAnnotSet(m_pAnnotationSet);
    m_pRTSEOutput->data()->setSurfSet(m_pSurfaceSet);
    m_pRTSEOutput->data()->setFwdSolution(m_pFwd);

    // start clustering
    QFuture<void> future = QtConcurrent::run(this, &RapMusicToolbox::doClustering);

}


//*************************************************************************************************************

void RapMusicToolbox::unload()
{

}


//*************************************************************************************************************

void RapMusicToolbox::calcFiffInfo()
{
    QMutexLocker locker(&m_qMutex);
    if(m_pFiffInfoEvoked && m_pFiffInfoForward)
    {
        qDebug() << "Fiff Infos available";

        m_qListPickChannels.clear();
        foreach (const QString &ch, m_pFiffInfoForward->ch_names)
        {
            if(m_pFiffInfoEvoked->ch_names.contains(ch))
                m_qListPickChannels << ch;
        }

        RowVectorXi sel = m_pFiffInfoEvoked->pick_channels(m_qListPickChannels);

        m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(m_pFiffInfoEvoked->pick_info(sel)));
    }

}


//*************************************************************************************************************

void RapMusicToolbox::doClustering()
{
    emit clusteringStarted();

    m_qMutex.lock();
    m_bFinishedClustering = false;
    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(*m_pAnnotationSet.data(), 40)));
    m_qMutex.unlock();

    finishedClustering();
}


//*************************************************************************************************************

void RapMusicToolbox::finishedClustering()
{
    m_qMutex.lock();
    m_bFinishedClustering = true;
    m_pFiffInfoForward = QSharedPointer<FiffInfoBase>(new FiffInfoBase(m_pClusteredFwd->info));
    m_qMutex.unlock();

    emit clusteringFinished();
}


//*************************************************************************************************************

bool RapMusicToolbox::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

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

bool RapMusicToolbox::stop()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    m_qMutex.lock();
    m_bIsRunning = false;

    if(m_bProcessData) // Only clear if buffers have been initialised
        m_qVecFiffEvoked.clear();

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    m_bReceiveData = false;

    m_qMutex.unlock();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType RapMusicToolbox::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString RapMusicToolbox::getName() const
{
    return "RTC-MUSIC";
}


//*************************************************************************************************************

QWidget* RapMusicToolbox::setupWidget()
{
    RapMusicToolboxSetupWidget* setupWidget = new RapMusicToolboxSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    if(!m_bFinishedClustering)
        setupWidget->setClusteringState();

    connect(this, &RapMusicToolbox::clusteringStarted, setupWidget, &RapMusicToolboxSetupWidget::setClusteringState);
    connect(this, &RapMusicToolbox::clusteringFinished, setupWidget, &RapMusicToolboxSetupWidget::setSetupState);

    return setupWidget;
}


//*************************************************************************************************************

void RapMusicToolbox::updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeEvokedSet> pRTE = pMeasurement.dynamicCast<RealTimeEvokedSet>();

    QMutexLocker locker(&m_qMutex);
    //MEG
    if(pRTE && m_bReceiveData)
    {
        //Fiff Information of the evoked
        if(!m_pFiffInfoEvoked)
            m_pFiffInfoEvoked = QSharedPointer<FiffInfo>(new FiffInfo(pRTE->getValue()->info));

        if(m_bProcessData)
            m_qVecFiffEvoked.push_back(pRTE->getValue()->pick_channels(m_qListPickChannels));
    }
}


//*************************************************************************************************************

void RapMusicToolbox::run()
{
    qint32 numDipolePairs = 1;

    //
    // start receiving data
    //
    m_qMutex.lock();
    m_bReceiveData = true;
    m_qMutex.unlock();

    //
    // Read Fiff Info
    //
    while(true)
    {
        {
            QMutexLocker locker(&m_qMutex);
            if(m_pFiffInfo)
                break;
        }
        calcFiffInfo();
        msleep(10);// Wait for fiff Info
    }

    m_pPwlRapMusic.reset();

    m_pPwlRapMusic = RapMusic::SPtr(new RapMusic(*m_pClusteredFwd, false, numDipolePairs));

    //
    // start processing data
    //
    m_qMutex.lock();
    m_bProcessData = true;
    m_qMutex.unlock();

    qint32 skip_count = 0;
    while(true)
    {
        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
                break;
        }

        m_qMutex.lock();
        qint32 t_evokedSize = m_qVecFiffEvoked.size();
        m_qMutex.unlock();

        if(t_evokedSize > 0)
        {
            if(m_pPwlRapMusic && ((skip_count % 10) == 0))
            {
                m_qMutex.lock();
                FiffEvoked t_fiffEvoked = m_qVecFiffEvoked[0].evoked.first();
                m_pPwlRapMusic->setStcAttr(t_fiffEvoked.data.cols()/4.0,0.0);
                m_qVecFiffEvoked.pop_front();
                m_qMutex.unlock();

                qDebug() << "m_pRapMusic->calculateInverse";

                MNESourceEstimate sourceEstimate = m_pPwlRapMusic->calculateInverse(t_fiffEvoked);
                m_pRTSEOutput->data()->setValue(sourceEstimate);
            }
            else
            {
                m_qMutex.lock();
                m_qVecFiffEvoked.pop_front();
                m_qMutex.unlock();
            }
            ++skip_count;
        }
    }
}
