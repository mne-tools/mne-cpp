//=============================================================================================================
/**
 * @file     averaging.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"
#include "FormFiles/averagingsetupwidget.h"

#include <disp/viewers/averagingsettingsview.h>
#include <disp/viewers/artifactsettingsview.h>

#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimemultisamplearray.h>

#include <rtprocessing/rtave.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_bIsRunning(false)
, m_bProcessData(false)
{
}

//=============================================================================================================

Averaging::~Averaging()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<IPlugin> Averaging::clone() const
{
    QSharedPointer<Averaging> pAveragingClone(new Averaging);
    return pAveragingClone;
}

//=============================================================================================================

void Averaging::unload()
{
}

//=============================================================================================================

bool Averaging::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    m_qMutex.lock();
    m_bIsRunning = true;
    m_qMutex.unlock();

    // Start threads
    QThread::start();

    return true;
}

//=============================================================================================================

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

//=============================================================================================================

IPlugin::PluginType Averaging::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Averaging::getName() const
{
    return "Averaging";
}

//=============================================================================================================

QWidget* Averaging::setupWidget()
{
    AveragingSetupWidget* setupWidget = new AveragingSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

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

            // Init the stim channels
            for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i) {
                if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                    m_mapStimChsIndexNames.insert(m_pFiffInfo->chs[i].ch_name,i);
                }
            }

            if(m_pAveragingSettingsView) {
                m_pAveragingSettingsView->setStimChannels(m_mapStimChsIndexNames);
            }

            if(m_pArtifactSettingsView) {
                m_pArtifactSettingsView->setChInfo(m_pFiffInfo->chs);
            }
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

//=============================================================================================================

void Averaging::init()
{
    // Input
    m_pAveragingInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "AveragingIn", "Averaging input data");
    connect(m_pAveragingInput.data(), &PluginInputConnector::notify, this, &Averaging::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pAveragingInput);

    // Output
    m_pAveragingOutput = PluginOutputData<RealTimeEvokedSet>::create(this, "AveragingOut", "Averaging Output Data");
    m_pAveragingOutput->data()->setName(QString("Plugin/%1").arg(this->getName()));//Provide name to auto store widget settings
    m_outputConnectors.append(m_pAveragingOutput);

    //Add control widgets to output data (will be used by QuickControlView by the measurements display)
    m_pAveragingSettingsView = AveragingSettingsView::SPtr::create(QString("Plugin/%1").arg(this->getName()));

    m_pAveragingSettingsView->setObjectName("group_tab_Averaging_Settings");

    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeNumAverages,
            this, &Averaging::onChangeNumAverages);
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
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::changeBaselineActive,
            this, &Averaging::onChangeBaselineActive);
    connect(m_pAveragingSettingsView.data(), &AveragingSettingsView::resetAverage,
            this, &Averaging::onResetAverage);

    m_pAveragingOutput->data()->addControlWidget(m_pAveragingSettingsView);

    m_pArtifactSettingsView = ArtifactSettingsView::SPtr::create(QString("Plugin/%1").arg(this->getName()));
    m_pArtifactSettingsView->setObjectName("group_tab_Averaging_Artifact");

    connect(m_pArtifactSettingsView.data(), &ArtifactSettingsView::changeArtifactThreshold,
            this, &Averaging::onChangeArtifactThreshold);

    m_pAveragingOutput->data()->addControlWidget(m_pArtifactSettingsView);
}

//=============================================================================================================

void Averaging::onChangeNumAverages(qint32 numAve)
{
    QMutexLocker locker(&m_qMutex);
    if(m_pRtAve) {
        m_pRtAve->setAverageNumber(numAve);
    }
}

//=============================================================================================================

void Averaging::onChangeStimChannel(const QString& sStimCh)
{
    QMutexLocker locker(&m_qMutex);

    if(m_pRtAve) {
        m_pRtAve->setTriggerChIndx(m_mapStimChsIndexNames[sStimCh]);
    }
}

//=============================================================================================================

void Averaging::onChangePreStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);

    if(!m_pFiffInfo) {
        return;
    }

    int iPreStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pAveragingOutput) {
        m_pAveragingOutput->data()->setNumPreStimSamples(iPreStimSamples);
    }

    if(m_pRtAve) {
        m_pRtAve->setPreStim(iPreStimSamples, mseconds);
    }
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);

    if(!m_pFiffInfo) {
        return;
    }

    int iPostStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setPostStim(iPostStimSamples, mseconds);
    }
}

//=============================================================================================================

void Averaging::onChangeArtifactThreshold(const QMap<QString,double>& mapThresholds)
{
    QMutexLocker locker(&m_qMutex);

    if(m_pRtAve) {
        m_pRtAve->setArtifactReduction(mapThresholds);
    }
}

//=============================================================================================================

void Averaging::onChangeBaselineFrom(qint32 fromMSeconds)
{
    QMutexLocker locker(&m_qMutex);

    if(!m_pFiffInfo) {
        return;
    }

    int iBaselineFromSamples = ((float)(fromMSeconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setBaselineFrom(iBaselineFromSamples, fromMSeconds);
    }
}

//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMSeconds)
{
    QMutexLocker locker(&m_qMutex);

    if(!m_pFiffInfo) {
        return;
    }

    int iBaselineToSamples = ((float)(toMSeconds)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setBaselineTo(iBaselineToSamples, toMSeconds);
    }
}

//=============================================================================================================

void Averaging::onChangeBaselineActive(bool state)
{
    QMutexLocker locker(&m_qMutex);
    if(m_pRtAve) {
        m_pRtAve->setBaselineActive(state);
    }
}

//=============================================================================================================

void Averaging::appendEvoked(const FIFFLIB::FiffEvokedSet& evokedSet,
                             const QStringList& lResponsibleTriggerTypes)
{
    m_qMutex.lock();
    m_qVecEvokedData.push_back(evokedSet);
    m_lResponsibleTriggerTypes = lResponsibleTriggerTypes;

    if(m_pAveragingSettingsView) {
        m_pAveragingSettingsView->setDetectedEpochs(evokedSet);
    }

    m_qMutex.unlock();
}

//=============================================================================================================

void Averaging::onResetAverage(bool state)
{
    Q_UNUSED(state)
    QMutexLocker locker(&m_qMutex);

    if(m_pRtAve) {
        m_pRtAve->reset();
    }
}

//=============================================================================================================

void Averaging::run()
{
    // Wait for fiff Info
    while(!m_pFiffInfo) {
        msleep(10);
    }

    if(m_mapStimChsIndexNames.isEmpty()) {
        qDebug() << "Averaging::run() - No stim channels were found. Averaging plugin was not started.";
        return;
    }

    int iCurrentStimChIdx = m_mapStimChsIndexNames.value(m_pAveragingSettingsView->getCurrentStimCh());

    if(!m_mapStimChsIndexNames.contains(m_pAveragingSettingsView->getCurrentStimCh())) {
        qDebug() << "Averaging::run() - Current stim channel is not present in data. Setting to first found stim channel instead.";
        iCurrentStimChIdx = m_mapStimChsIndexNames.first();
    }

    // Init Real-Time average    
    int iPreStimSamples = ((float)m_pAveragingSettingsView->getPreStimSeconds()/1000)*m_pFiffInfo->sfreq;
    int iPostStimSamples = ((float)m_pAveragingSettingsView->getPostStimSeconds()/1000)*m_pFiffInfo->sfreq;
    int iBaselineFromSamples = ((float)m_pAveragingSettingsView->getBaselineFromSeconds()/1000)*m_pFiffInfo->sfreq;
    int iBaselineToSamples = ((float)m_pAveragingSettingsView->getBaselineToSeconds()/1000)*m_pFiffInfo->sfreq;

    m_pRtAve = RtAve::SPtr::create(m_pAveragingSettingsView->getNumAverages(),
                                   iPreStimSamples,
                                   iPostStimSamples,
                                   m_pAveragingSettingsView->getBaselineFromSeconds(),
                                   m_pAveragingSettingsView->getBaselineToSeconds(),
                                   iCurrentStimChIdx,
                                   m_pFiffInfo);
    m_pRtAve->setBaselineFrom(iBaselineFromSamples, m_pAveragingSettingsView->getBaselineFromSeconds());
    m_pRtAve->setBaselineTo(iBaselineToSamples, m_pAveragingSettingsView->getBaselineToSeconds());
    m_pRtAve->setBaselineActive(m_pAveragingSettingsView->getDoBaselineCorrection());
    m_pRtAve->setArtifactReduction(m_pArtifactSettingsView->getThresholdMap());

    connect(m_pRtAve.data(), &RtAve::evokedStim,
            this, &Averaging::appendEvoked);

    m_bProcessData = true;

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
