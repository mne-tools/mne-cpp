//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivity.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Definition of the NeuronalConnectivity class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <base/string_utils.h>
#include "FormFiles/neuronalconnectivitysetupwidget.h"

#include <connectivity/connectivity.h>
#include <dsp/rt/rt_connectivity.h>

#include <disp/viewers/connectivitysettingsview.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne_bem.h>

#include <disp/viewers/connectivitysettingsview.h>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEURONALCONNECTIVITYPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace mne::disp;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivity::NeuronalConnectivity()
: m_iDownSample(1)
, m_iNumberAverages(10)
, m_iNumberBadChannels(0)
, m_fFreqBandLow(7.0f)
, m_fFreqBandHigh(13.0f)
, m_sAvrType("1")
, m_pTrialBuffer(std::make_shared<CircularBuffer<Eigen::MatrixXd>>(40))
{
    m_connectivitySettings.set_frequency_bins(0, 100);
}

//=============================================================================================================

NeuronalConnectivity::~NeuronalConnectivity()
{
    if (this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> NeuronalConnectivity::clone() const
{
    QSharedPointer<NeuronalConnectivity> pNeuronalConnectivityClone(new NeuronalConnectivity);
    return pNeuronalConnectivityClone;
}

//=============================================================================================================

void NeuronalConnectivity::init()
{
    // Input
    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "NeuronalConnectivityInSource", "NeuronalConnectivity source input data");
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify,
            this, &NeuronalConnectivity::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NeuronalConnectivityInSensor", "NeuronalConnectivity sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &NeuronalConnectivity::updateRTMSA, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    m_pRTEVSInput = PluginInputData<RealTimeEvokedSet>::create(this, "NeuronalConnectivityInSensorEvoked", "NeuronalConnectivity evoked input data");
    connect(m_pRTEVSInput.data(), &PluginInputConnector::notify, this,
            &NeuronalConnectivity::updateRTEV, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTEVSInput);

    // Output
    m_pRTCEOutput = PluginOutputData<RealTimeConnectivityEstimate>::create(this, "NeuronalConnectivityOut", "NeuronalConnectivity output data");
    m_outputConnectors.append(m_pRTCEOutput);
    m_pRTCEOutput->measurementData()->setName(this->getName());
}

//=============================================================================================================

void NeuronalConnectivity::initPluginControlWidgets()
{
    QList<QWidget*> plControlWidgets;

    ConnectivitySettingsView* pConnectivitySettingsView = new ConnectivitySettingsView(QString("MNESCAN/%1").arg(this->getName()));
    connect(this, &NeuronalConnectivity::guiModeChanged,
            pConnectivitySettingsView, &ConnectivitySettingsView::setGuiMode);
    pConnectivitySettingsView->setObjectName("group_tab_Settings_Connectivity");

    //Add control widgets to output data (will be used by QuickControlView by the measurements display)
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::connectivityMetricChanged,
            this, &NeuronalConnectivity::onMetricChanged);
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::numberTrialsChanged,
            this, &NeuronalConnectivity::onNumberTrialsChanged);
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::triggerTypeChanged,
            this, &NeuronalConnectivity::onTriggerTypeChanged);
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::freqBandChanged,
            this, &NeuronalConnectivity::onFrequencyBandChanged);
    connect(this, &NeuronalConnectivity::responsibleTriggerTypesChaged,
            pConnectivitySettingsView, &ConnectivitySettingsView::setTriggerTypes);

    plControlWidgets.append(pConnectivitySettingsView);

    onFrequencyBandChanged(pConnectivitySettingsView->getLowerFreq(), pConnectivitySettingsView->getUpperFreq());
    onMetricChanged(pConnectivitySettingsView->getConnectivityMetric());
    onWindowTypeChanged(pConnectivitySettingsView->getWindowType());
    onNumberTrialsChanged(pConnectivitySettingsView->getNumberTrials());
    onTriggerTypeChanged(pConnectivitySettingsView->getTriggerType());

    emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

    m_bPluginControlWidgetsInit = true;
}

//=============================================================================================================

void NeuronalConnectivity::unload()
{
}

//=============================================================================================================

bool NeuronalConnectivity::start()
{
    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool NeuronalConnectivity::stop()
{
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType NeuronalConnectivity::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString NeuronalConnectivity::getName() const
{
    return "Connectivity";
}

//=============================================================================================================

QWidget* NeuronalConnectivity::setupWidget()
{
    NeuronalConnectivitySetupWidget* setupWidget = new NeuronalConnectivitySetupWidget(this); //widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void NeuronalConnectivity::updateSource(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>()) {
        //Fiff information
        if (!m_pFiffInfo) {
            m_pFiffInfo = pRTSE->getFiffInfo();

            m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

            //Init output - Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
            m_pRTCEOutput->measurementData()->setAnnotSet(pRTSE->getAnnotSet());
            m_pRTCEOutput->measurementData()->setSurfSet(pRTSE->getSurfSet());
            m_pRTCEOutput->measurementData()->setFwdSolution(pRTSE->getFwdSolution());
            m_pRTCEOutput->measurementData()->setFiffInfo(m_pFiffInfo);

            // Generate network nodes
            m_connectivitySettings.setNodePositions(*pRTSE->getFwdSolution(), *pRTSE->getSurfSet());
        }

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        for (qint32 i = 0; i < pRTSE->getValue().size(); ++i) {
            // Only the samples from the stimulus on
            const auto& stc = *pRTSE->getValue()[i];
            Eigen::Index iZeroIdx = 0;
            while (iZeroIdx < stc.times.cols() && stc.times(iZeroIdx) < 0) {
                ++iZeroIdx;
            }
            pushTrial(stc.data.rightCols(stc.data.cols() - iZeroIdx));
        }
    }
}

//=============================================================================================================

void NeuronalConnectivity::updateRTMSA(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if (!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

            //Set 3D sensor surface for visualization
            std::shared_ptr<MNEBem> pSensorSurfaceVV = std::make_shared<MNEBem>((QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif").toStdU16String());
            m_pRTCEOutput->measurementData()->setSensorSurface(pSensorSurfaceVV);
            m_pRTCEOutput->measurementData()->setFiffInfo(m_pFiffInfo);

            //Generate node vertices
            generateNodeVertices();
            m_iNumberBadChannels = static_cast<int>(m_pFiffInfo->bads.size());
        }

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        if (m_pFiffInfo) {
            //Generate node vertices because the number of bad channels changed
            if (m_iNumberBadChannels != static_cast<int>(pRTMSA->info()->bads.size())) {
                m_pFiffInfo = pRTMSA->info();
                generateNodeVertices();
                m_iNumberBadChannels = static_cast<int>(m_pFiffInfo->bads.size());
            }

            for (const MatrixXd& block : pRTMSA->getMultiSampleArray()) {
                pushTrial(block(m_vecPicks, placeholders::all));
            }
        }
    }
}

//=============================================================================================================

void NeuronalConnectivity::updateRTEV(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeEvokedSet> pRTEV = pMeasurement.dynamicCast<RealTimeEvokedSet>()) {
        std::shared_ptr<FiffEvokedSet> pFiffEvokedSet = pRTEV->getValue();
        QStringList lResponsibleTriggerTypes = pRTEV->getResponsibleTriggerTypes();

        emit responsibleTriggerTypesChaged(lResponsibleTriggerTypes);

        if (!pFiffEvokedSet || !lResponsibleTriggerTypes.contains(m_sAvrType)) {
            return;
        }

        //qDebug() << "NeuronalConnectivity::updateRTEV - Found trigger" << m_sAvrType;

        //Fiff Information of the evoked
        if (!m_pFiffInfo && pFiffEvokedSet->evoked.size() > 0) {
            for (int i = 0; i < std::ssize(pFiffEvokedSet->evoked); ++i) {
                if (pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                    m_pFiffInfo = std::make_shared<FiffInfo>(pFiffEvokedSet->info);

                    m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

                    //Set 3D sensor surface for visualization
                    std::shared_ptr<MNEBem> pSensorSurfaceVV = std::make_shared<MNEBem>((QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif").toStdU16String());
                    m_pRTCEOutput->measurementData()->setSensorSurface(pSensorSurfaceVV);
                    m_pRTCEOutput->measurementData()->setFiffInfo(m_pFiffInfo);

                    //Generate node vertices
                    generateNodeVertices();
                    m_iNumberBadChannels = static_cast<int>(m_pFiffInfo->bads.size());

                    break;
                }
            }
        }

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        if (m_pFiffInfo) {
            for (const FiffEvoked& evoked : pFiffEvokedSet->evoked) {
                if (evoked.comment == m_sAvrType) {
                    pushTrial(evoked.data(m_vecPicks, placeholders::all));
                    break;
                }
            }
        }
    }
}

//=============================================================================================================

void NeuronalConnectivity::generateNodeVertices()
{
    if (!m_pFiffInfo) {
        qDebug() << "NeuronalConnectivity::generateNodeVertices - FiffInfo is Null. Returning.";
        return;
    }

    QString sCoilType = "grad";
    QString sChType = "meg";

    StringList exclude = m_pFiffInfo->bads;
    for (const std::string& name : m_pFiffInfo->ch_names)
        if (name.find("EOG") != std::string::npos)
            exclude.push_back(name);

    if (sChType.contains("EEG", Qt::CaseInsensitive)) {
        m_vecPicks = m_pFiffInfo->pick_types(false, true, false, {}, exclude);
    } else if (sCoilType.contains("grad", Qt::CaseInsensitive)) {
        // Only pick every second gradiometer which are not marked as bad.
        RowVectorXi picksTmp = m_pFiffInfo->pick_types("grad", false, false);
        m_vecPicks.resize(0);

        for (int i = 0; i < picksTmp.cols() - 1; i += 2) {
            if (!std::ranges::contains(m_pFiffInfo->bads, m_pFiffInfo->ch_names.at(picksTmp(i)))) {
                m_vecPicks.conservativeResize(m_vecPicks.cols() + 1);
                m_vecPicks(m_vecPicks.cols() - 1) = picksTmp(i);
            }
        }
    } else if (sCoilType.contains("mag", Qt::CaseInsensitive)) {
        m_vecPicks = m_pFiffInfo->pick_types("mag", false, false, {}, exclude);
    }

    // The processing thread starts over with the new channels
    QMutexLocker locker(&m_mutex);
    m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);
    m_connectivitySettings.setNodePositions(*m_pFiffInfo, m_vecPicks);
    m_bSettingsChanged = true;
}

//=============================================================================================================

void NeuronalConnectivity::pushTrial(Eigen::MatrixXd trial)
{
    // Blocks until the processing thread has room
    while (!m_pTrialBuffer->push(std::move(trial)) && !isInterruptionRequested()) { // NOLINT(bugprone-use-after-move): a timed-out push does not move
    }
}

//=============================================================================================================

void NeuronalConnectivity::run()
{
    // Wait for fiff info
    while (!isInterruptionRequested()) {
        m_mutex.lock();
        if (m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        msleep(100);
    }
    if (isInterruptionRequested()) {
        return;
    }

    std::optional<RtConnectivity> rtConnectivity;
    int skip_count = 0;
    MatrixXd trial;
    std::vector<Network> networks;

    while (!isInterruptionRequested()) {
        m_mutex.lock();
        if (!rtConnectivity || m_bSettingsChanged) {
            rtConnectivity.emplace(m_connectivitySettings, m_iNumberAverages);
            m_bSettingsChanged = false;
        } else {
            // A new method reuses the kept spectra of the trials in the window
            rtConnectivity->settings().setConnectivityMethods(m_sConnectivityMethods);
            rtConnectivity->set_trial_count(m_iNumberAverages);
        }
        const float freqLow = m_fFreqBandLow;
        const float freqHigh = m_fFreqBandHigh;
        const bool republish = std::exchange(m_bRepublish, false);
        const int downSample = std::max(1, m_iDownSample);
        m_mutex.unlock();

        if (m_pTrialBuffer->pop(trial)) {
            networks = rtConnectivity->update(trial);
            if (++skip_count % downSample != 0) {
                continue;
            }
        } else if (!republish) {
            continue;
        } else if (!rtConnectivity->settings().empty()) {
            networks = rtConnectivity->networks();
        }

        //Send the data to the connected plugins and the online display
        for (Network network : networks) {
            network.set_frequency_range(freqLow, freqHigh);
            network.normalize();
            m_pRTCEOutput->measurementData()->setValue(network);
        }
    }
}

//=============================================================================================================

void NeuronalConnectivity::onMetricChanged(const QString& sMetric)
{
    if (std::ranges::contains(m_sConnectivityMethods, sMetric.toStdString())) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_sConnectivityMethods = {sMetric.toStdString()};
    m_connectivitySettings.setConnectivityMethods(m_sConnectivityMethods);
    m_bRepublish = true;
}

//=============================================================================================================

void NeuronalConnectivity::onNumberTrialsChanged(int iNumberTrials)
{
    QMutexLocker locker(&m_mutex);
    m_iNumberAverages = iNumberTrials;
}

//=============================================================================================================

void NeuronalConnectivity::onWindowTypeChanged(const QString& windowType)
{
    QMutexLocker locker(&m_mutex);
    if (m_connectivitySettings.getWindowType() != windowType.toStdString()) {
        m_connectivitySettings.setWindowType(windowType.toStdString());
        m_bSettingsChanged = true;
    }
}

//=============================================================================================================

void NeuronalConnectivity::onTriggerTypeChanged(const QString& triggerType)
{
    if (triggerType != m_sAvrType) {
        QMutexLocker locker(&m_mutex);
        m_sAvrType = triggerType;
        m_bSettingsChanged = true;
    }
}

//=============================================================================================================

void NeuronalConnectivity::onFrequencyBandChanged(float fFreqLow, float fFreqHigh)
{
    QMutexLocker locker(&m_mutex);
    m_fFreqBandLow = fFreqLow;
    m_fFreqBandHigh = fFreqHigh;
    m_bRepublish = true;
}

//=============================================================================================================

QString NeuronalConnectivity::getBuildInfo()
{
    return QString(NEURONALCONNECTIVITYPLUGIN::buildDateTime()) + QString(" - ") + QString(NEURONALCONNECTIVITYPLUGIN::buildHash());
}
