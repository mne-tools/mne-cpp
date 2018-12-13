//=============================================================================================================
/**
* @file     averaging.cpp
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
* @brief    Definition of the Averaging class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"
#include "FormFiles/averagingsetupwidget.h"

#include <disp/viewers/averagingsettingsview.h>

#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimemultisamplearray.h>

#include <rtprocessing/rtave.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace FIFFLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_bIsRunning(false)
, m_bProcessData(false)
, m_iPreStimSeconds(100)
, m_iPostStimSeconds(400)
, m_dArtifactThresholdFirst(150)
, m_iArtifactThresholdSecond(-6)
, m_iBaselineFromSeconds(0)
, m_iBaselineToSeconds(0)
, m_iNumAverages(10)
, m_iAverageMode(0)
, m_bDoBaselineCorrection(false)
, m_bDoArtifactThresholdReduction(false)
, m_bDoArtifactVarianceReduction(false)
, m_dArtifactVariance(0.5)
, m_iPreStimSamples(0)
, m_iPostStimSamples(0)
, m_iBaselineFromSamples(0)
, m_iBaselineToSamples(0)
, m_iStimChanIdx(0)
, m_iStimChan(0)
{
}


//*************************************************************************************************************

Averaging::~Averaging()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Averaging::clone() const
{
    QSharedPointer<Averaging> pAveragingClone(new Averaging);
    return pAveragingClone;
}


//*************************************************************************************************************

void Averaging::unload()
{
    // Store Settings
    QSettings settings;
    settings.setValue(QString("Plugin/%1/preStimSeconds").arg(this->getName()), m_iPreStimSeconds);
    settings.setValue(QString("Plugin/%1/postStimSeconds").arg(this->getName()), m_iPostStimSeconds);
    settings.setValue(QString("Plugin/%1/baselineFromSamples").arg(this->getName()), m_iBaselineFromSamples);
    settings.setValue(QString("Plugin/%1/baselineToSamples").arg(this->getName()), m_iBaselineToSamples);

    settings.setValue(QString("Plugin/%1/numAverages").arg(this->getName()), m_iNumAverages);
    settings.setValue(QString("Plugin/%1/stimChannelIdx").arg(this->getName()), m_iStimChanIdx);
    settings.setValue(QString("Plugin/%1/stimChannel").arg(this->getName()), m_iStimChan);
    settings.setValue(QString("Plugin/%1/averageMode").arg(this->getName()), m_iAverageMode);

    settings.setValue(QString("Plugin/%1/doArtifactThresholdReduction").arg(this->getName()), m_bDoArtifactThresholdReduction);
    settings.setValue(QString("Plugin/%1/doArtifactVarianceReduction").arg(this->getName()), m_bDoArtifactVarianceReduction);
    settings.setValue(QString("Plugin/%1/artifactThresholdFirst").arg(this->getName()), m_dArtifactThresholdFirst);
    settings.setValue(QString("Plugin/%1/artifactThresholdSecond").arg(this->getName()), m_iArtifactThresholdSecond);
    settings.setValue(QString("Plugin/%1/artifactVariance").arg(this->getName()), m_dArtifactVariance);

    settings.setValue(QString("Plugin/%1/baselineFromSeconds").arg(this->getName()), m_iBaselineFromSeconds);
    settings.setValue(QString("Plugin/%1/baselineToSeconds").arg(this->getName()), m_iBaselineToSeconds);
    settings.setValue(QString("Plugin/%1/baselineToSamples").arg(this->getName()), m_iBaselineToSamples);
    settings.setValue(QString("Plugin/%1/baselineFromSamples").arg(this->getName()), m_iBaselineFromSamples);

    settings.setValue(QString("Plugin/%1/doBaselineCorrection").arg(this->getName()), m_bDoBaselineCorrection);
}


//*************************************************************************************************************

bool Averaging::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_qMutex.lock();
    m_bIsRunning = true;
    m_qMutex.unlock();

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Averaging::stop()
{
    //Wait until this thread is stopped
    m_qMutex.lock();
    m_bIsRunning = false;

    if(m_bProcessData) {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pAveragingBuffer->releaseFromPop();
        m_pAveragingBuffer->releaseFromPush();
        m_pAveragingBuffer->clear();
//        m_pRTMSAOutput->data()->clear();
    }

    m_qMutex.unlock();
    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Averaging::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString Averaging::getName() const
{
    return "Averaging";
}


//*************************************************************************************************************

QWidget* Averaging::setupWidget()
{
    AveragingSetupWidget* setupWidget = new AveragingSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Averaging::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pAveragingBuffer) {
            m_pAveragingBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

         //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();
        }

        // Append new data
        if(m_bProcessData) {
            for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                if(m_pRtAve) {
                    m_pAveragingBuffer->push(&pRTMSA->getMultiSampleArray()[i]);
                }
            }
        }
    }
}


//*************************************************************************************************************

void Averaging::init()
{
    // Load Settings
    QSettings settings;
    m_iPreStimSeconds = settings.value(QString("Plugin/%1/preStimSeconds").arg(this->getName()), 100).toInt();
    m_iPostStimSeconds = settings.value(QString("Plugin/%1/postStimSeconds").arg(this->getName()), 400).toInt();
    m_iPreStimSamples = settings.value(QString("Plugin/%1/preStimSeconds").arg(this->getName()), 100).toInt();
    m_iPostStimSamples = settings.value(QString("Plugin/%1/postStimSeconds").arg(this->getName()), 400).toInt();

    m_iBaselineFromSeconds = settings.value(QString("Plugin/%1/baselineFromSeconds").arg(this->getName()), 0).toInt();
    m_iBaselineToSeconds = settings.value(QString("Plugin/%1/baselineToSeconds").arg(this->getName()), 0).toInt();
    m_iBaselineFromSamples = settings.value(QString("Plugin/%1/baselineFromSamples").arg(this->getName()), 0).toInt();
    m_iBaselineToSamples = settings.value(QString("Plugin/%1/baselineToSamples").arg(this->getName()), 0).toInt();

    m_bDoArtifactThresholdReduction = settings.value(QString("Plugin/%1/doArtifactThresholdReduction").arg(this->getName()), false).toBool();
    m_bDoArtifactVarianceReduction = settings.value(QString("Plugin/%1/doArtifactVarianceReduction").arg(this->getName()), false).toBool();
    m_dArtifactThresholdFirst = settings.value(QString("Plugin/%1/artifactThresholdFirst").arg(this->getName()), m_dArtifactThresholdFirst).toDouble();
    m_iArtifactThresholdSecond = settings.value(QString("Plugin/%1/artifactThresholdSecond").arg(this->getName()), m_iArtifactThresholdSecond).toInt();
    m_dArtifactVariance = settings.value(QString("Plugin/%1/artifactVariance").arg(this->getName()), 3).toInt();

    m_iNumAverages = settings.value(QString("Plugin/%1/numAverages").arg(this->getName()), 10).toInt();
    m_iStimChanIdx = settings.value(QString("Plugin/%1/stimChannelIdX").arg(this->getName()), 0).toInt();
    m_iAverageMode = settings.value(QString("Plugin/%1/averageMode").arg(this->getName()), 0).toInt();
    m_iStimChan = settings.value(QString("Plugin/%1/stimChannel").arg(this->getName()), 0).toInt();

    m_bDoBaselineCorrection = settings.value(QString("Plugin/%1/doBaselineCorrection").arg(this->getName()), false).toBool();

    // Input
    m_pAveragingInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "AveragingIn", "Averaging input data");
    connect(m_pAveragingInput.data(), &PluginInputConnector::notify, this, &Averaging::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pAveragingInput);

    // Output
    m_pAveragingOutput = PluginOutputData<RealTimeEvokedSet>::create(this, "AveragingOut", "Averaging Output Data");
    m_pAveragingOutput->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pAveragingOutput);

    //Add control widgets to output data (will be used by QuickControlView by the measurements display)
    m_pAveragingSettingsView = AveragingSettingsView::SPtr::create(Q_NULLPTR,
                                                                   m_pFiffInfo,
                                                                   m_qListStimChs,
                                                                   m_iStimChan,
                                                                   m_iNumAverages,
                                                                   m_iPreStimSeconds,
                                                                   m_iPostStimSeconds,
                                                                   m_bDoArtifactThresholdReduction,
                                                                   m_bDoArtifactVarianceReduction,
                                                                   m_dArtifactThresholdFirst,
                                                                   m_iArtifactThresholdSecond,
                                                                   m_dArtifactVariance,
                                                                   m_bDoBaselineCorrection,
                                                                   m_iBaselineFromSeconds,
                                                                   m_iBaselineToSeconds);
    m_pAveragingSettingsView->setObjectName("group_tab_Averaging_Settings");

    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeNumAverages,
            this, &Averaging::onChangeNumAverages);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeArtifactThreshold,
            this, &Averaging::onChangeArtifactThreshold);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeArtifactVariance,
            this, &Averaging::onChangeArtifactVariance);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeBaselineFrom,
            this, &Averaging::onChangeBaselineFrom);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeBaselineTo,
            this, &Averaging::onChangeBaselineTo);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changePostStim,
            this, &Averaging::onChangePostStim);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changePreStim,
            this, &Averaging::onChangePreStim);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeStimChannel,
            this, &Averaging::onChangeStimChannel);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeArtifactThresholdReductionActive,
            this, &Averaging::onChangeArtifactThresholdReductionActive);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeArtifactVarianceReductionActive,
            this, &Averaging::onChangeArtifactVarianceReductionActive);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeArtifactVariance,
            this, &Averaging::onChangeArtifactVariance);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeBaselineActive,
            this, &Averaging::onChangeBaselineActive);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::resetAverage,
            this, &Averaging::onResetAverage);

    m_pAveragingOutput->data()->addControlWidget(m_pAveragingSettingsView);
}


//*************************************************************************************************************

void Averaging::onChangeNumAverages(qint32 numAve)
{
    QMutexLocker locker(&m_qMutex);
    m_iNumAverages = numAve;
    if(m_pRtAve) {
        m_pRtAve->setAverageNumber(numAve);
    }
}


//*************************************************************************************************************

void Averaging::onChangeStimChannel(qint32 index)
{
    QMutexLocker locker(&m_qMutex);

    m_iStimChanIdx = m_qListStimChs.at(index);
    m_iStimChan = m_pAveragingSettingsView->getStimChannelIdx();

    if(m_pRtAve) {
        m_pRtAve->setTriggerChIndx(m_iStimChanIdx);
    }
}


//*************************************************************************************************************

void Averaging::onChangePreStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);
    if(mseconds<10) {
        mseconds=10;
    }

    m_iPreStimSeconds = mseconds;
    m_iPreStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

    //qDebug()<<"m_iPreStimSamples "<<m_iPreStimSamples;

    if(m_pAveragingOutput) {
        m_pAveragingOutput->data()->setNumPreStimSamples(m_iPreStimSamples);
    }

    if(m_pRtAve) {
        m_pRtAve->setPreStim(m_iPreStimSamples, m_iPreStimSeconds);
    }
}


//*************************************************************************************************************

void Averaging::onChangePostStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);
    if(mseconds<10) {
        mseconds=10;
    }

    m_iPostStimSeconds = mseconds;
    m_iPostStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setPostStim(m_iPostStimSamples, m_iPostStimSeconds);
    }
}


//*************************************************************************************************************

void Averaging::onChangeArtifactThreshold(double thresholdFirst, int thresholdSecond)
{
    QMutexLocker locker(&m_qMutex);

    m_iArtifactThresholdSecond = thresholdSecond;
    m_dArtifactThresholdFirst = thresholdFirst;

    if(m_pRtAve) {
        m_pRtAve->setArtifactReduction(m_bDoArtifactThresholdReduction, m_dArtifactThresholdFirst * pow(10, m_iArtifactThresholdSecond), m_bDoArtifactVarianceReduction, m_dArtifactVariance);
    }
}


//*************************************************************************************************************

void Averaging::onChangeArtifactThresholdReductionActive(bool state)
{
    QMutexLocker locker(&m_qMutex);

    m_bDoArtifactThresholdReduction = state;

    if(m_pRtAve) {
        m_pRtAve->setArtifactReduction(m_bDoArtifactThresholdReduction, m_dArtifactThresholdFirst * pow(10, m_iArtifactThresholdSecond), m_bDoArtifactVarianceReduction, m_dArtifactVariance);
    }
}


//*************************************************************************************************************

void Averaging::onChangeArtifactVariance(double dVariance)
{
    QMutexLocker locker(&m_qMutex);

    m_dArtifactVariance = dVariance;

    if(m_pRtAve) {
        m_pRtAve->setArtifactReduction(m_bDoArtifactThresholdReduction,
                                       m_dArtifactThresholdFirst * pow(10, m_iArtifactThresholdSecond),
                                       m_bDoArtifactVarianceReduction,
                                       m_dArtifactVariance);
    }
}


//*************************************************************************************************************

void Averaging::onChangeArtifactVarianceReductionActive(bool state)
{
    QMutexLocker locker(&m_qMutex);

    m_bDoArtifactVarianceReduction = state;

    if(m_pRtAve) {
        m_pRtAve->setArtifactReduction(m_bDoArtifactThresholdReduction,
                                       m_dArtifactThresholdFirst * pow(10, m_iArtifactThresholdSecond),
                                       m_bDoArtifactVarianceReduction,
                                       m_dArtifactVariance);
    }
}


//*************************************************************************************************************

void Averaging::onChangeBaselineFrom(qint32 fromMSeconds)
{
    QMutexLocker locker(&m_qMutex);
    m_iBaselineFromSeconds = fromMSeconds;
    m_iBaselineFromSamples = ((float)(fromMSeconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setBaselineFrom(m_iBaselineFromSamples, m_iBaselineFromSeconds);
    }
}


//*************************************************************************************************************

void Averaging::onChangeBaselineTo(qint32 toMSeconds)
{
    QMutexLocker locker(&m_qMutex);
    m_iBaselineToSeconds = toMSeconds;
    m_iBaselineToSamples = ((float)(toMSeconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setBaselineTo(m_iBaselineToSamples, m_iBaselineToSeconds);
    }
}


//*************************************************************************************************************

void Averaging::onChangeBaselineActive(bool state)
{
    QMutexLocker locker(&m_qMutex);
    m_bDoBaselineCorrection = state;

    if(m_pRtAve) {
        m_pRtAve->setBaselineActive(m_bDoBaselineCorrection);
    }
}


//*************************************************************************************************************

void Averaging::appendEvoked(const FIFFLIB::FiffEvokedSet& evokedSet,
                             const QStringList& lResponsibleTriggerTypes)
{
    m_qMutex.lock();
    m_qVecEvokedData.push_back(evokedSet);
    m_lResponsibleTriggerTypes = lResponsibleTriggerTypes;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void Averaging::onResetAverage(bool state)
{
    Q_UNUSED(state)
    QMutexLocker locker(&m_qMutex);

    if(m_pRtAve) {
        m_pRtAve->reset();
    }
}


//*************************************************************************************************************

void Averaging::run()
{
    // Wait for fiff Info
    while(!m_pFiffInfo) {
        msleep(10);
    }

    m_iPreStimSamples = ((float)m_iPreStimSeconds/1000)*m_pFiffInfo->sfreq;
    m_iPostStimSamples = ((float)m_iPostStimSeconds/1000)*m_pFiffInfo->sfreq;

    // Init the stim channels
    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
            //qDebug() << "Stim" << i << "Name" << m_pFiffInfo->chs[i].ch_name;
            m_qListStimChs.append(i);
        }
    }

    m_pAveragingSettingsView->setStimChannels(m_pFiffInfo,
                                              m_qListStimChs,
                                              m_iStimChan);

    // If initital stim channel index does not match current data choose the first one
    if(!m_qListStimChs.contains(m_iStimChanIdx) &&
       !m_qListStimChs.isEmpty()) {
        m_iStimChanIdx = m_qListStimChs.first();
    }

    m_bProcessData = true;

    // Init Real-Time average
    m_pRtAve = RtAve::SPtr::create(m_iNumAverages,
                                   m_iPreStimSamples,
                                   m_iPostStimSamples,
                                   m_iBaselineFromSeconds,
                                   m_iBaselineToSeconds,
                                   m_qListStimChs.at(m_iStimChan),
                                   m_pFiffInfo);
    m_pRtAve->setBaselineFrom(m_iBaselineFromSamples, m_iBaselineFromSeconds);
    m_pRtAve->setBaselineTo(m_iBaselineToSamples, m_iBaselineToSeconds);
    m_pRtAve->setBaselineActive(m_bDoBaselineCorrection);
    m_pRtAve->setArtifactReduction(m_bDoArtifactThresholdReduction,
                                   m_dArtifactThresholdFirst * pow(10, m_iArtifactThresholdSecond),
                                   m_bDoArtifactVarianceReduction,
                                   m_dArtifactVariance);

    connect(m_pRtAve.data(), &RtAve::evokedStim,
            this, &Averaging::appendEvoked);

    while(true) {
        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
                break;
        }

        bool doProcessing = false;

        {
            QMutexLocker locker(&m_qMutex);
            doProcessing = m_bProcessData;
        }

        if(doProcessing) {
            m_pRtAve->append(m_pAveragingBuffer->pop());

            // Dispatch the inputs
            m_qMutex.lock();
            if(!m_qVecEvokedData.isEmpty()) {
                FiffEvokedSet t_fiffEvokedSet = m_qVecEvokedData.takeFirst();

                m_pAveragingOutput->data()->setValue(t_fiffEvokedSet,
                                                     m_pFiffInfo,
                                                     m_lResponsibleTriggerTypes);

            }
            m_qMutex.unlock();

        }
    }

    m_pRtAve->stop();
}
