//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     averaging.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the Averaging class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "averaging.h"

#include "FormFiles/averagingsetupwidget.h"
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/artifact_panel.h>
#include <mne/viz/panels/averaging_panel.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

// The artifact view adds an "Active" switch to the peak-to-peak limits; no limits switch the rejection off
std::map<std::string, double> toRejection(const std::map<std::string, double>& thresholds)
{
    std::map<std::string, double> result;
    if (const auto active = thresholds.find("Active"); active == thresholds.end() || active->second == 0.0) {
        return result;
    }
    for (const auto& [key, value] : thresholds) {
        if (key != "Active") {
            result.emplace(key, value);
        }
    }
    return result;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pBlockBuffer(std::make_shared<mne::CircularBuffer<MatrixXd>>(40))
{
}

//=============================================================================================================

Averaging::~Averaging()
{
    if (this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Averaging::clone() const
{
    auto pAveragingClone = QSharedPointer<Averaging>::create();
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
    AveragingSetupWidget* setupWidget = new AveragingSetupWidget(this); //widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Averaging::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if (!m_pFiffInfo) {
            QMutexLocker locker(&m_qMutex);
            m_pFiffInfo = pRTMSA->info();

            // Init the stim channels
            for (qint32 i = 0; i < std::ssize(m_pFiffInfo->chs); ++i) {
                if (m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                    m_mapStimChsIndexNames.insert_or_assign(m_pFiffInfo->chs[i].ch_name, i);
                }
            }
        }

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        // The buffer keeps a copy, since the measurement reuses its blocks
        for (const MatrixXd& block : pRTMSA->getMultiSampleArray()) {
            while (!m_pBlockBuffer->push(block) && !isInterruptionRequested()) {
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
    m_pAveragingOutput->measurementData()->setName(this->getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pAveragingOutput);
}

//=============================================================================================================

void Averaging::initPluginControlWidgets()
{
    if (m_pFiffInfo) {
        std::vector<std::unique_ptr<QWidget>> controlWidgets;

        //Add control widgets to output data (will be used by QuickControlView by the measurements display)
        auto averaging = std::make_shared<AveragingPanel>();
        AveragingPanel::Callbacks callbacks;
        callbacks.num_averages_changed = [this](int value) {
            onChangeNumAverages(value);
        };
        callbacks.baseline_from_changed = [this](int value) {
            onChangeBaselineFrom(value);
        };
        callbacks.baseline_to_changed = [this](int value) {
            onChangeBaselineTo(value);
        };
        callbacks.post_stim_changed = [this](int value) {
            onChangePostStim(value);
        };
        callbacks.pre_stim_changed = [this](int value) {
            onChangePreStim(value);
        };
        callbacks.stim_channel_changed = [this](const std::string& channel) {
            onChangeStimChannel(channel);
        };
        callbacks.baseline_active_changed = [this](bool active) {
            onChangeBaselineActive(active);
        };
        callbacks.reset = [this] {
            onResetAverage(false);
        };
        averaging->set_callbacks(std::move(callbacks));
        PanelWidget* pAveragingSettingsView = makeControlWidget<PanelWidget>(controlWidgets, averaging, QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Averaging::guiModeChanged,
                pAveragingSettingsView, &PanelWidget::setGuiMode);
        pAveragingSettingsView->setObjectName("group_tab_Settings_Averaging");
        connect(this, &Averaging::stimChannelsChanged,
                pAveragingSettingsView, [averaging](const std::map<std::string, int>& channels) {
                    averaging->set_stim_channels(channels);
                });
        connect(this, &Averaging::evokedSetChanged,
                pAveragingSettingsView, [averaging](const FiffEvokedSet& evokedSet) {
                    averaging->set_detected_epochs(evokedSet);
                });

        emit stimChannelsChanged(m_mapStimChsIndexNames);

        auto artifact = std::make_shared<ArtifactPanel>();
        artifact->set_thresholds_callback([this](const std::map<std::string, double>& thresholds) {
            onChangeArtifactThreshold(thresholds);
        });
        PanelWidget* pArtifactSettingsView = makeControlWidget<PanelWidget>(controlWidgets, artifact, QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &Averaging::guiModeChanged,
                pArtifactSettingsView, &PanelWidget::setGuiMode);
        pArtifactSettingsView->setObjectName("group_tab_Settings_Artifact");
        connect(this, &Averaging::fiffChInfoChanged,
                pArtifactSettingsView, [artifact](const std::vector<FiffChInfo>& channels) {
                    artifact->set_channels(channels);
                });

        emit fiffChInfoChanged(m_pFiffInfo->chs);

        offerControlWidgets(std::move(controlWidgets));

        // Init the averaging settings
        const auto currentStimCh = m_mapStimChsIndexNames.find(averaging->current_stim_channel());
        int iCurrentStimChIdx = currentStimCh != m_mapStimChsIndexNames.end() ? currentStimCh->second : 0;

        if (currentStimCh == m_mapStimChsIndexNames.end()) {
            Log::debug("Averaging::run() - Current stim channel is not present in data. Setting to first found stim channel instead.");
            iCurrentStimChIdx = m_mapStimChsIndexNames.empty() ? 0 : m_mapStimChsIndexNames.begin()->second;
        }

        QMutexLocker locker(&m_qMutex);
        m_settings.pre_stim = static_cast<int>(averaging->pre_stim() / 1000.0 * m_pFiffInfo->sfreq);
        m_settings.post_stim = static_cast<int>(averaging->post_stim() / 1000.0 * m_pFiffInfo->sfreq);
        m_settings.trigger_channel = iCurrentStimChIdx;
        m_settings.averages = averaging->num_averages();
        m_settings.reject = toRejection(artifact->thresholds());
        m_baseline = {static_cast<float>(averaging->baseline_from()) / 1000.0f,
                      static_cast<float>(averaging->baseline_to()) / 1000.0f};
        m_bBaselineActive = averaging->baseline_correction();
        applyBaseline();

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void Averaging::onChangeNumAverages(qint32 numAve)
{
    QMutexLocker locker(&m_qMutex);
    m_settings.averages = numAve;
    m_bSettingsChanged = true;
}

//=============================================================================================================

void Averaging::onChangeStimChannel(const std::string& sStimCh)
{
    QMutexLocker locker(&m_qMutex);
    const auto stimCh = m_mapStimChsIndexNames.find(sStimCh);
    m_settings.trigger_channel = stimCh != m_mapStimChsIndexNames.end() ? stimCh->second : -1;
    m_bSettingsChanged = true;
}

//=============================================================================================================

void Averaging::onChangePreStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);

    if (!m_pFiffInfo) {
        return;
    }

    m_settings.pre_stim = static_cast<int>(mseconds / 1000.0 * m_pFiffInfo->sfreq);
    m_bSettingsChanged = true;

    if (m_pAveragingOutput) {
        m_pAveragingOutput->measurementData()->setNumPreStimSamples(m_settings.pre_stim);
    }
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{
    QMutexLocker locker(&m_qMutex);

    if (!m_pFiffInfo) {
        return;
    }

    m_settings.post_stim = static_cast<int>(mseconds / 1000.0 * m_pFiffInfo->sfreq);
    m_bSettingsChanged = true;
}

//=============================================================================================================

void Averaging::onChangeArtifactThreshold(const std::map<std::string, double>& mapThresholds)
{
    QMutexLocker locker(&m_qMutex);
    m_settings.reject = toRejection(mapThresholds);
    m_bSettingsChanged = true;
}

//=============================================================================================================

void Averaging::onChangeBaselineFrom(qint32 fromMS)
{
    QMutexLocker locker(&m_qMutex);
    m_baseline.first = static_cast<float>(fromMS) / 1000.0f;
    applyBaseline();
}

//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMSeconds)
{
    QMutexLocker locker(&m_qMutex);
    m_baseline.second = static_cast<float>(toMSeconds) / 1000.0f;
    applyBaseline();
}

//=============================================================================================================

void Averaging::onChangeBaselineActive(bool state)
{
    QMutexLocker locker(&m_qMutex);
    m_bBaselineActive = state;
    applyBaseline();
}

//=============================================================================================================

void Averaging::applyBaseline()
{
    m_settings.baseline = m_bBaselineActive ? FiffEvoked::Baseline(m_baseline) : std::nullopt;
    m_bSettingsChanged = true;
}

//=============================================================================================================

void Averaging::onResetAverage(bool state)
{
    Q_UNUSED(state)
    QMutexLocker locker(&m_qMutex);
    m_bResetRequested = true;
}

//=============================================================================================================

void Averaging::run()
{
    std::optional<RtAveraging> averaging;
    MatrixXd block;

    while (!isInterruptionRequested()) {
        if (!m_pBlockBuffer->pop(block)) {
            continue;
        }

        m_qMutex.lock();
        if (!averaging) {
            averaging.emplace(m_pFiffInfo, m_settings);
        } else if (m_bSettingsChanged) {
            averaging->set_settings(m_settings);
        }
        m_bSettingsChanged = false;
        if (std::exchange(m_bResetRequested, false)) {
            averaging->reset();
        }
        m_qMutex.unlock();

        const std::vector<int> changed = averaging->update(block);
        if (changed.empty()) {
            continue;
        }
        QStringList lResponsibleTriggerTypes;
        for (const int code : changed) {
            lResponsibleTriggerTypes << QString::number(code);
        }
        m_pAveragingOutput->measurementData()->setValue(averaging->evoked(), m_pFiffInfo, lResponsibleTriggerTypes);
        emit evokedSetChanged(averaging->evoked());
    }
}

//=============================================================================================================

QString Averaging::getBuildInfo()
{
    return QString(AVERAGINGPLUGIN::buildDateTime()) + QString(" - ") + QString(AVERAGINGPLUGIN::buildHash());
}
