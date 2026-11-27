//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     artifactsettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of the ArtifactSettingsView per-modality threshold panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifactsettingsview.h"
#include "../detail/scoped_flag.h"
#include "settingspanel.h"

#include <mne/fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** Channel types with a threshold, in the order of the panel. */
constexpr std::array<const char*, 6> kChannelTypes = {"grad", "mag", "eeg", "ecg", "emg", "eog"};

/** Key of the flag that a channel type is present. */
std::string presentKey(const std::string& type)
{
    return "present/" + type;
}

/** Key of the mantissa of a threshold; also its QSettings key. */
std::string mantissaKey(const std::string& type)
{
    return "artifactThresholdsFirst/" + type;
}

/** Key of the decimal exponent of a threshold; also its QSettings key. */
std::string exponentKey(const std::string& type)
{
    return "artifactThresholdsSecond/" + type;
}

/** The channel type of a channel that gets a threshold, or none. */
const char* channelType(const FiffChInfo& channel)
{
    switch (channel.kind) {
        case FIFFV_MEG_CH:
            return channel.unit == FIFF_UNIT_T_M ? "grad" : channel.unit == FIFF_UNIT_T ? "mag"
                                                                                        : nullptr;
        case FIFFV_EEG_CH:
            return "eeg";
        case FIFFV_EOG_CH:
            return "eog";
        case FIFFV_EMG_CH:
            return "emg";
        case FIFFV_ECG_CH:
            return "ecg";
        default:
            return nullptr;
    }
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ArtifactSettingsView::ArtifactSettingsView(const QString& sSettingsPath,
                                           const std::vector<FiffChInfo>& fiffChInfoList,
                                           QWidget* parent)
: AbstractView(parent)
, m_pForm(std::make_shared<viz::SettingsForm>("Artifact Rejection Settings"))
, m_fiffChInfoList(fiffChInfoList)
{
    m_sSettingsPath = sSettingsPath;
    qRegisterMetaType<std::map<std::string, double>>("std::map<std::string, double>");

    this->setWindowTitle("Artifact Rejection Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    // One row per channel type, shown while channels of the type are present
    m_pForm->add_flag("hasChannelTypes");
    viz::SettingField active;
    active.key = "doArtifactThresholdReduction";
    active.kind = viz::SettingField::Kind::Bool;
    active.label = "Activate artifact rejection";
    active.default_value = false;
    active.visible_if = "hasChannelTypes";
    m_pForm->add(active);
    for (const std::string type : kChannelTypes) {
        m_pForm->add_flag(presentKey(type));

        viz::SettingField mantissa;
        mantissa.key = mantissaKey(type);
        mantissa.kind = viz::SettingField::Kind::Double;
        mantissa.label = type;
        mantissa.default_value = 1.0;
        mantissa.maximum = 100000.0;
        mantissa.prefix = "+/-";
        mantissa.visible_if = presentKey(type);
        m_pForm->add(mantissa);

        viz::SettingField exponent;
        exponent.key = exponentKey(type);
        exponent.kind = viz::SettingField::Kind::Int;
        exponent.default_value = -1;
        exponent.minimum = -10000;
        exponent.maximum = 0;
        exponent.decimals = 0;
        exponent.prefix = "e";
        exponent.visible_if = presentKey(type);
        m_pForm->add(exponent);
    }

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (!m_bSilent && (key == "doArtifactThresholdReduction" || key.starts_with("artifactThresholds"))) {
            onChangeArtifactThreshold();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_pPanel = new SettingsPanel(m_pForm, this);
    layout->addWidget(m_pPanel);

    loadSettings();
    redrawGUI();
    updateThresholdMap();
}

//=============================================================================================================

ArtifactSettingsView::~ArtifactSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void ArtifactSettingsView::setChInfo(const std::vector<mne::FiffChInfo>& fiffChInfoList)
{
    m_fiffChInfoList = fiffChInfoList;

    redrawGUI();
    onChangeArtifactThreshold();
}

//=============================================================================================================

std::map<std::string, double> ArtifactSettingsView::getThresholdMap()
{
    return m_mapThresholds;
}

//=============================================================================================================

void ArtifactSettingsView::setThresholdMap(const std::map<std::string, double>& mapThresholds)
{
    m_mapThresholds = mapThresholds;

    redrawGUI();
    onChangeArtifactThreshold();
}

//=============================================================================================================

bool ArtifactSettingsView::getDoArtifactThresholdRejection()
{
    return m_pForm->get<bool>("hasChannelTypes") && m_pForm->get<bool>("doArtifactThresholdReduction");
}

//=============================================================================================================

void ArtifactSettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    m_pPanel->saveSettings(m_sSettingsPath + QString("/ArtifactSettingsView"));
}

//=============================================================================================================

void ArtifactSettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    const detail::ScopedFlag silent(m_bSilent);
    m_pPanel->loadSettings(m_sSettingsPath + QString("/ArtifactSettingsView"));
}

//=============================================================================================================

void ArtifactSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::redrawGUI()
{
    std::vector<std::string> present;
    present.reserve(kChannelTypes.size());
    for (const FiffChInfo& channel : m_fiffChInfoList) {
        if (const char* type = channelType(channel)) {
            present.emplace_back(type);
        }
    }

    const detail::ScopedFlag silent(m_bSilent);
    for (const std::string type : kChannelTypes) {
        m_pForm->set_value(presentKey(type), std::ranges::contains(present, type));
    }
    m_pForm->set_value("hasChannelTypes", !present.empty());
}

//=============================================================================================================

void ArtifactSettingsView::updateThresholdMap()
{
    m_mapThresholds.clear();
    m_mapThresholds["Active"] = getDoArtifactThresholdRejection() ? 1.0 : 0.0;
    for (const std::string type : kChannelTypes) {
        if (m_pForm->get<bool>(presentKey(type))) {
            m_mapThresholds[type] = m_pForm->get<double>(mantissaKey(type)) * std::pow(10, m_pForm->get<int>(exponentKey(type)));
        }
    }
}

//=============================================================================================================

void ArtifactSettingsView::onChangeArtifactThreshold()
{
    updateThresholdMap();

    emit changeArtifactThreshold(m_mapThresholds);

    saveSettings();
}

//=============================================================================================================

void ArtifactSettingsView::clearView()
{
}
