//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     averagingsettingsview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2015
 * @brief    Implementation of the AveragingSettingsView settings panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsettingsview.h"
#include "../detail/layout_reset.h"
#include "../detail/scoped_flag.h"
#include "settingspanel.h"

#include <mne/fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** Adds a field edited in a spin box that hands its value over when editing finishes. */
void addMilliseconds(viz::SettingsForm& form, const std::string& key, const std::string& label, int value, int minimum,
                     const std::string& group, const std::string& prefix = {})
{
    viz::SettingField field;
    field.key = key;
    field.kind = viz::SettingField::Kind::Int;
    field.label = label;
    field.default_value = value;
    field.minimum = minimum;
    field.maximum = 10000;
    field.step = 10;
    field.decimals = 0;
    field.prefix = prefix;
    field.unit = " ms";
    field.group = group;
    field.commit = viz::SettingField::Commit::WhenFinished;
    form.add(field);
}

using IntSignal = std::pair<std::string_view, void (AveragingSettingsView::*)(qint32)>;
using BoolSignal = std::pair<std::string_view, void (AveragingSettingsView::*)(bool)>;

/** Number fields and the signal that reports them. */
constexpr std::array<IntSignal, 5> kIntSignals = {{{"preStimSeconds", &AveragingSettingsView::changePreStim},
                                                   {"postStimSeconds", &AveragingSettingsView::changePostStim},
                                                   {"baselineFromSeconds", &AveragingSettingsView::changeBaselineFrom},
                                                   {"baselineToSeconds", &AveragingSettingsView::changeBaselineTo},
                                                   {"numAverages", &AveragingSettingsView::changeNumAverages}}};

/** Check boxes and buttons and the signal that reports them. */
constexpr std::array<BoolSignal, 5> kBoolSignals = {{{"doBaselineCorrection", &AveragingSettingsView::changeBaselineActive},
                                                     {"dropRejected", &AveragingSettingsView::changeDropActive},
                                                     {"autoCompute", &AveragingSettingsView::setAutoCompute},
                                                     {"reset", &AveragingSettingsView::resetAverage},
                                                     {"compute", &AveragingSettingsView::calculateAverage}}};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingSettingsView::AveragingSettingsView(const QString& sSettingsPath,
                                             const std::map<std::string, int>& mapStimChsIndexNames,
                                             QWidget* parent)
: AbstractView(parent)
, m_pForm(std::make_shared<viz::SettingsForm>("Averaging Settings"))
, m_mapStimChsIndexNames(mapStimChsIndexNames)
{
    m_sSettingsPath = sSettingsPath;

    this->setWindowTitle("Averaging Settings");
    this->setMinimumWidth(330);

    // Mode flags the conditions of the fields refer to
    m_pForm->add_flag("offline");
    m_pForm->add_flag("hasDetectedTrials");

    const std::string general = "General";
    viz::SettingField stimChannel;
    stimChannel.key = "currentStimChannel";
    stimChannel.kind = viz::SettingField::Kind::Choice;
    stimChannel.label = "Stimulus channel";
    stimChannel.group = general;
    stimChannel.visible_if = "!offline";
    // The stored channel may be unknown until the stimulus channels arrive; loadSettings() keeps it aside
    stimChannel.persistent = false;
    m_pForm->add(stimChannel);

    viz::SettingField numAverages;
    numAverages.key = "numAverages";
    numAverages.kind = viz::SettingField::Kind::Int;
    numAverages.label = "Number of trials";
    numAverages.default_value = 10;
    numAverages.minimum = 1;
    numAverages.maximum = 1000;
    numAverages.decimals = 0;
    numAverages.group = general;
    numAverages.visible_if = "!offline";
    numAverages.commit = viz::SettingField::Commit::WhenFinished;
    m_pForm->add(numAverages);

    addMilliseconds(*m_pForm, "preStimSeconds", "Pre-stimulus", 100, 0, general, "-");
    addMilliseconds(*m_pForm, "postStimSeconds", "Post-stimulus", 400, 10, general);

    // The baseline ranges follow the epoch window once the settings are loaded
    const std::string baseline = "Baseline correction";
    m_pForm->add_bool("doBaselineCorrection", "Activate", false, baseline);
    addMilliseconds(*m_pForm, "baselineFromSeconds", "Baseline min", 0, -10000, baseline);
    addMilliseconds(*m_pForm, "baselineToSeconds", "Baseline max", 0, -10000, baseline);

    viz::SettingField detectedTrials;
    detectedTrials.key = "detectedTrials";
    detectedTrials.kind = viz::SettingField::Kind::Custom;
    detectedTrials.group = "Detected Trials";
    detectedTrials.visible_if = "hasDetectedTrials";
    m_pForm->add(detectedTrials);

    viz::SettingField reset;
    reset.key = "reset";
    reset.kind = viz::SettingField::Kind::Action;
    reset.label = "Reset";
    reset.visible_if = "!offline";
    m_pForm->add(reset);

    viz::SettingField dropRejected;
    dropRejected.key = "dropRejected";
    dropRejected.kind = viz::SettingField::Kind::Bool;
    dropRejected.label = "Drop rejected";
    dropRejected.default_value = false;
    dropRejected.visible_if = "offline";
    dropRejected.persistent = false;
    m_pForm->add(dropRejected);

    viz::SettingField autoCompute = dropRejected;
    autoCompute.key = "autoCompute";
    autoCompute.label = "Live average based on events";
    m_pForm->add(autoCompute);

    viz::SettingField compute;
    compute.key = "compute";
    compute.kind = viz::SettingField::Kind::Action;
    compute.label = "Save";
    compute.visible_if = "offline";
    m_pForm->add(compute);

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        onFormChanged(key);
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_pPanel = new SettingsPanel(m_pForm, this);
    layout->addWidget(m_pPanel);

    // The detected trials replace the layout of their custom field
    if (auto* pTrials = m_pPanel->customWidget("detectedTrials")) {
        detail::resetLayout(pTrials);
        m_pDetectedTrials = new QGridLayout(pTrials);
        m_pDetectedTrials->setContentsMargins(0, 0, 0, 0);
    }

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

AveragingSettingsView::~AveragingSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::setStimChannels(const std::map<std::string, int>& mapStimChsIndexNames)
{
    if (!mapStimChsIndexNames.empty()) {
        m_mapStimChsIndexNames = mapStimChsIndexNames;
        fillStimChannels();
    }
}

//=============================================================================================================

void AveragingSettingsView::fillStimChannels()
{
    const detail::ScopedFlag silent(m_bSilent);
    std::vector<viz::SettingChoice> choices;
    choices.reserve(m_mapStimChsIndexNames.size());
    for (const auto& [name, index] : m_mapStimChsIndexNames) {
        choices.push_back({name, name});
    }
    m_pForm->set_choices("currentStimChannel", std::move(choices));
    m_pForm->set_value("currentStimChannel", m_sCurrentStimChan);
}

//=============================================================================================================

std::string AveragingSettingsView::getCurrentStimCh()
{
    return m_sCurrentStimChan;
}

//=============================================================================================================

bool AveragingSettingsView::getDoBaselineCorrection()
{
    return m_pForm->get<bool>("doBaselineCorrection");
}

//=============================================================================================================

int AveragingSettingsView::getNumAverages()
{
    return m_pForm->get<int>("numAverages");
}

//=============================================================================================================

int AveragingSettingsView::getBaselineFromSeconds()
{
    return m_pForm->get<int>("baselineFromSeconds");
}

//=============================================================================================================

int AveragingSettingsView::getBaselineToSeconds()
{
    return m_pForm->get<int>("baselineToSeconds");
}

//=============================================================================================================

int AveragingSettingsView::getPreStimMSeconds()
{
    return m_pForm->get<int>("preStimSeconds");
}

//=============================================================================================================

int AveragingSettingsView::getPostStimMSeconds()
{
    return m_pForm->get<int>("postStimSeconds");
}

//=============================================================================================================

int AveragingSettingsView::getStimChannelIdx()
{
    const auto it = m_mapStimChsIndexNames.find(m_pForm->get<std::string>("currentStimChannel"));
    return it == m_mapStimChsIndexNames.end() ? 0 : it->second;
}

//=============================================================================================================

void AveragingSettingsView::redrawGUI()
{
    if (!m_mapStimChsIndexNames.empty()) {
        fillStimChannels();
    }

    // The baseline lies in the epoch window
    const detail::ScopedFlag silent(m_bSilent);
    const int preStim = getPreStimMSeconds();
    const int postStim = getPostStimMSeconds();
    m_pForm->set_range("baselineFromSeconds", -preStim, postStim);
    m_pForm->set_range("baselineToSeconds", -preStim, postStim);

    setWindowFlags(Qt::WindowStaysOnTopHint);
}

//=============================================================================================================

void AveragingSettingsView::setDetectedEpochs(const FiffEvokedSet& evokedSet)
{
    m_pForm->set_value("hasDetectedTrials", !evokedSet.evoked.empty());
    if (evokedSet.evoked.empty() || !m_pDetectedTrials) {
        return;
    }

    // The widgets go right away: the view is redrawn synchronously and looked up by name
    detail::clearLayout(m_pDetectedTrials);

    auto* table = m_pDetectedTrials->parentWidget();
    m_pDetectedTrials->addWidget(new QLabel("Type", table), 0, 0);
    m_pDetectedTrials->addWidget(new QLabel("#", table), 0, 1);

    // Show only a maximum of 10 average types
    const int count = std::min(static_cast<int>(std::ssize(evokedSet.evoked)), 10);
    for (int i = 0; i < count; i++) {
        m_pDetectedTrials->addWidget(new QLabel(QString::fromStdString(evokedSet.evoked.at(i).comment), table), i + 1, 0);
        m_pDetectedTrials->addWidget(new QLabel(QString::number(evokedSet.evoked.at(i).nave), table), i + 1, 1);
    }
}

//=============================================================================================================

void AveragingSettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    m_pPanel->saveSettings(m_sSettingsPath + QString("/AveragingSettingsView"));
    QSettings settings("MNECPP");
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/currentStimChannel"), QString::fromStdString(m_sCurrentStimChan));
}

//=============================================================================================================

void AveragingSettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    const detail::ScopedFlag silent(m_bSilent);
    // The stored baseline is checked against the stored epoch window, not against the current one
    m_pForm->set_range("baselineFromSeconds", -10000, 10000);
    m_pForm->set_range("baselineToSeconds", -10000, 10000);
    m_pPanel->loadSettings(m_sSettingsPath + QString("/AveragingSettingsView"));

    const int preStim = getPreStimMSeconds();
    const int postStim = getPostStimMSeconds();
    if (const int from = getBaselineFromSeconds(); from < -preStim || from > postStim) {
        m_pForm->set_value("baselineFromSeconds", -preStim);
    }
    if (const int to = getBaselineToSeconds(); to > postStim || to < -preStim) {
        m_pForm->set_value("baselineToSeconds", 0);
    }

    m_pForm->set_range("baselineFromSeconds", -preStim, postStim);
    m_pForm->set_range("baselineToSeconds", -preStim, postStim);

    QSettings settings("MNECPP");
    m_sCurrentStimChan = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/currentStimChannel"), "STI 014").toString().toStdString();
}

//=============================================================================================================

void AveragingSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AveragingSettingsView::updateProcessingMode(ProcessingMode mode)
{
    m_pForm->set_value("offline", mode == ProcessingMode::Offline);
}

//=============================================================================================================

void AveragingSettingsView::onFormChanged(const std::string& key)
{
    if (m_bSilent) {
        return;
    }

    // Fields that only report their value, and actions that report false like the clicked buttons did
    for (const auto& [name, signal] : kBoolSignals) {
        if (key == name) {
            emit(this->*signal)(m_pForm->get<bool>(key));
            if (m_pForm->value(key)) {
                saveSettings();
            }
            return;
        }
    }

    // The epoch window bounds the baseline, the baseline bounds itself
    const int mSeconds = m_pForm->get<int>(key);
    const auto range = [this](const std::string& field) {
        const viz::SettingField* f = m_pForm->field(field);
        return std::pair{static_cast<int>(f->minimum), static_cast<int>(f->maximum)};
    };
    if (key == "preStimSeconds") {
        m_pForm->set_range("baselineToSeconds", -mSeconds, range("baselineToSeconds").second);
        m_pForm->set_range("baselineFromSeconds", -mSeconds, range("baselineFromSeconds").second);
    } else if (key == "postStimSeconds") {
        m_pForm->set_range("baselineToSeconds", range("baselineToSeconds").first, mSeconds);
        m_pForm->set_range("baselineFromSeconds", range("baselineFromSeconds").first, mSeconds);
    } else if (key == "baselineFromSeconds") {
        m_pForm->set_range("baselineToSeconds", mSeconds, range("baselineToSeconds").second);
    } else if (key == "baselineToSeconds") {
        m_pForm->set_range("baselineFromSeconds", range("baselineFromSeconds").first, mSeconds);
    }

    if (key == "currentStimChannel") {
        m_sCurrentStimChan = m_pForm->get<std::string>(key);
        emit changeStimChannel(m_sCurrentStimChan);
    } else if (const auto it = std::ranges::find(kIntSignals, key, &IntSignal::first); it != kIntSignals.end()) {
        emit(this->*it->second)(mSeconds);
    } else {
        return;
    }

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::clearView()
{
}

//=============================================================================================================

bool AveragingSettingsView::getAutoComputeStatus()
{
    return m_pForm->get<bool>("autoCompute");
}

//=============================================================================================================

QString AveragingSettingsView::getCurrentSelectGroup()
{
    return QString();
}

//=============================================================================================================

void AveragingSettingsView::clearSelectionGroup()
{
}
