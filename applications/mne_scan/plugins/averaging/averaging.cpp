//=============================================================================================================
/**
 * @file     averaging.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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

#include <rtprocessing/rtaveraging.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pCircularBuffer(CircularBuffer<FIFFLIB::FiffEvokedSet>::SPtr::create(40))
{
}

//=============================================================================================================

Averaging::~Averaging()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Averaging::clone() const
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
    QThread::start();

    return true;
}

//=============================================================================================================

bool Averaging::stop()
{    
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Averaging::getType() const
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
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
         //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            // Init the stim channels
            for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i) {
                if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                    m_mapStimChsIndexNames.insert(m_pFiffInfo->chs[i].ch_name,i);
                }
            }
        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        // Append new data
        MatrixXd matData;

        if(m_pFiffInfo) {
            for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                if(m_pRtAve) {
                    // This extra copy is necessary since the referenced data is getting deleted as soon as
                    // m_pRtAve->append() returns. m_pRtAve->append() returns without a copy since it communicates
                    // via signals with the worker thread of RtCov.
                    matData = pRTMSA->getMultiSampleArray()[i];
                    m_pRtAve->append(matData);
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
    connect(m_pAveragingInput.data(), &PluginInputConnector::notify,
            this, &Averaging::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pAveragingInput);

    // Output
    m_pAveragingOutput = PluginOutputData<RealTimeEvokedSet>::create(this, "AveragingOut", "Averaging Output Data");
    m_pAveragingOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pAveragingOutput);
}

//=============================================================================================================

void Averaging::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        //Add control widgets to output data (will be used by QuickControlView by the measurements display)
        AveragingSettingsView* pAveragingSettingsView = new AveragingSettingsView(QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Averaging::guiModeChanged,
                pAveragingSettingsView, &AveragingSettingsView::setGuiMode);
        pAveragingSettingsView->setObjectName("group_tab_Settings_Averaging");

        connect(pAveragingSettingsView, &AveragingSettingsView::changeNumAverages,
                this, &Averaging::onChangeNumAverages);
        connect(pAveragingSettingsView, &AveragingSettingsView::changeBaselineFrom,
                this, &Averaging::onChangeBaselineFrom);
        connect(pAveragingSettingsView, &AveragingSettingsView::changeBaselineTo,
                this, &Averaging::onChangeBaselineTo);
        connect(pAveragingSettingsView, &AveragingSettingsView::changePostStim,
                this, &Averaging::onChangePostStim);
        connect(pAveragingSettingsView, &AveragingSettingsView::changePreStim,
                this, &Averaging::onChangePreStim);
        connect(pAveragingSettingsView, &AveragingSettingsView::changeStimChannel,
                this, &Averaging::onChangeStimChannel);
        connect(pAveragingSettingsView, &AveragingSettingsView::changeBaselineActive,
                this, &Averaging::onChangeBaselineActive);
        connect(pAveragingSettingsView, &AveragingSettingsView::resetAverage,
                this, &Averaging::onResetAverage);
        connect(this, &Averaging::stimChannelsChanged,
                pAveragingSettingsView, &AveragingSettingsView::setStimChannels);
        connect(this, &Averaging::evokedSetChanged,
                pAveragingSettingsView, &AveragingSettingsView::setDetectedEpochs);

        emit stimChannelsChanged(m_mapStimChsIndexNames);

        plControlWidgets.append(pAveragingSettingsView);

        ArtifactSettingsView* pArtifactSettingsView = new ArtifactSettingsView(QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Averaging::guiModeChanged,
                pArtifactSettingsView, &ArtifactSettingsView::setGuiMode);
        pArtifactSettingsView->setObjectName("group_tab_Settings_Artifact");

        connect(pArtifactSettingsView, &ArtifactSettingsView::changeArtifactThreshold,
                this, &Averaging::onChangeArtifactThreshold);
        connect(this, &Averaging::fiffChInfoChanged,
                pArtifactSettingsView, &ArtifactSettingsView::setChInfo);

        emit fiffChInfoChanged(m_pFiffInfo->chs);

        plControlWidgets.append(pArtifactSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        // Init RtAveraging
        int iCurrentStimChIdx = m_mapStimChsIndexNames.value(pAveragingSettingsView->getCurrentStimCh());

        if(!m_mapStimChsIndexNames.contains(pAveragingSettingsView->getCurrentStimCh())) {
            qDebug() << "Averaging::run() - Current stim channel is not present in data. Setting to first found stim channel instead.";
            iCurrentStimChIdx = m_mapStimChsIndexNames.first();
        }

        // Init Real-Time average
        int iPreStimSamples = ((float)pAveragingSettingsView->getPreStimMSeconds()/1000)*m_pFiffInfo->sfreq;
        int iPostStimSamples = ((float)pAveragingSettingsView->getPostStimMSeconds()/1000)*m_pFiffInfo->sfreq;
        int iBaselineFromSamples = ((float)pAveragingSettingsView->getBaselineFromSeconds()/1000)*m_pFiffInfo->sfreq;
        int iBaselineToSamples = ((float)pAveragingSettingsView->getBaselineToSeconds()/1000)*m_pFiffInfo->sfreq;

        m_pRtAve = RtAveraging::SPtr::create(pAveragingSettingsView->getNumAverages(),
                                       iPreStimSamples,
                                       iPostStimSamples,
                                       pAveragingSettingsView->getBaselineFromSeconds(),
                                       pAveragingSettingsView->getBaselineToSeconds(),
                                       iCurrentStimChIdx,
                                       m_pFiffInfo);

        connect(m_pRtAve.data(), &RtAveraging::evokedStim,
                this, &Averaging::onNewEvokedSet);

        m_pRtAve->setBaselineFrom(iBaselineFromSamples, pAveragingSettingsView->getBaselineFromSeconds());
        m_pRtAve->setBaselineTo(iBaselineToSamples, pAveragingSettingsView->getBaselineToSeconds());
        m_pRtAve->setBaselineActive(pAveragingSettingsView->getDoBaselineCorrection());
        m_pRtAve->setArtifactReduction(pArtifactSettingsView->getThresholdMap());

        m_bPluginControlWidgetsInit = true;
    }
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
        m_pAveragingOutput->measurementData()->setNumPreStimSamples(iPreStimSamples);
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

void Averaging::onChangeBaselineFrom(qint32 fromMS)
{
    QMutexLocker locker(&m_qMutex);

    if(!m_pFiffInfo) {
        return;
    }

    int iBaselineFromSamples = ((float)(fromMS)/1000)*m_pFiffInfo->sfreq;

    if(m_pRtAve) {
        m_pRtAve->setBaselineFrom(iBaselineFromSamples, fromMS);
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

void Averaging::onNewEvokedSet(const FIFFLIB::FiffEvokedSet& evokedSet,
                               const QStringList& lResponsibleTriggerTypes)
{
    if(!this->isRunning()) {
        return;
    }

    while(!m_pCircularBuffer->push(evokedSet)) {
        //Do nothing until the circular buffer is ready to accept new data again
    }

    emit evokedSetChanged(evokedSet);

    QMutexLocker locker(&m_qMutex);
    m_lResponsibleTriggerTypes = lResponsibleTriggerTypes;
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
    FIFFLIB::FiffEvokedSet evokedSet;
    QStringList lResponsibleTriggerTypes;

    while(!isInterruptionRequested()){
        if(m_pCircularBuffer->pop(evokedSet)) {
            m_qMutex.lock();
            lResponsibleTriggerTypes = m_lResponsibleTriggerTypes;
            m_qMutex.unlock();

            m_pAveragingOutput->measurementData()->setValue(evokedSet,
                                                 m_pFiffInfo,
                                                 lResponsibleTriggerTypes);
        }
    }
}

//=============================================================================================================

QString Averaging::getBuildDateTime()
{
    return QString(BUILDINFO::timestamp());
}
