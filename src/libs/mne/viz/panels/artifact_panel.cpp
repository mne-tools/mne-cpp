//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     artifact_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ArtifactPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifact_panel.h"
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kActive = "doArtifactThresholdReduction";
constexpr const char* kHasTypes = "hasChannelTypes";

/** Channel types with a threshold, in the order of the panel. */
constexpr std::array<const char*, 6> kChannelTypes = {"grad", "mag", "eeg", "ecg", "emg", "eog"};

/** Key of the flag that a channel type is present. */
std::string present_key(const std::string& type)
{
    return "present/" + type;
}

/** Key of the mantissa of a threshold. */
std::string mantissa_key(const std::string& type)
{
    return "artifactThresholdsFirst/" + type;
}

/** Key of the decimal exponent of a threshold. */
std::string exponent_key(const std::string& type)
{
    return "artifactThresholdsSecond/" + type;
}

/** The channel type of a channel that gets a threshold, or none. */
const char* channel_type(const mne::FiffChInfo& channel)
{
    switch (channel.kind) {
        case FIFFV_MEG_CH:
            if (channel.unit == FIFF_UNIT_T_M) {
                return "grad";
            }
            return channel.unit == FIFF_UNIT_T ? "mag" : nullptr;
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

ArtifactPanel::ArtifactPanel(const std::vector<mne::FiffChInfo>& channels)
: Panel("ArtifactSettingsView", "Artifact Rejection Settings")
{
    set_width(330);
    SettingsForm& f = form();
    f.add_flag(kHasTypes);
    SettingField activeField;
    activeField.key = kActive;
    activeField.kind = SettingField::Kind::Bool;
    activeField.label = "Activate artifact rejection";
    activeField.default_value = false;
    activeField.visible_if = kHasTypes;
    f.add(std::move(activeField));
    for (const std::string type : kChannelTypes) {
        f.add_flag(present_key(type));
        SettingField mantissa;
        mantissa.key = mantissa_key(type);
        mantissa.kind = SettingField::Kind::Double;
        mantissa.label = type;
        mantissa.default_value = 1.0;
        mantissa.maximum = 100000.0;
        mantissa.prefix = "+/-";
        mantissa.visible_if = present_key(type);
        f.add(std::move(mantissa));
        SettingField exponent;
        exponent.key = exponent_key(type);
        exponent.kind = SettingField::Kind::Int;
        exponent.default_value = -1;
        exponent.minimum = -10000;
        exponent.maximum = 0;
        exponent.decimals = 0;
        exponent.prefix = "e";
        exponent.visible_if = present_key(type);
        f.add(std::move(exponent));
    }
    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>&) {
        if (!m_updating && (key == kActive || key.starts_with("artifactThresholds"))) {
            report();
        }
    });
    const bool updating = std::exchange(m_updating, true);
    set_channels(channels);
    m_updating = updating;
}

//=============================================================================================================

ArtifactPanel::~ArtifactPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void ArtifactPanel::set_channels(const std::vector<mne::FiffChInfo>& channels)
{
    std::vector<std::string> present;
    present.reserve(kChannelTypes.size());
    for (const mne::FiffChInfo& channel : channels) {
        if (const char* type = channel_type(channel)) {
            present.emplace_back(type);
        }
    }
    {
        const bool updating = std::exchange(m_updating, true);
        for (const std::string type : kChannelTypes) {
            form().set_value(present_key(type), std::ranges::find(present, type) != present.end());
        }
        form().set_value(kHasTypes, !present.empty());
        m_updating = updating;
    }
    if (!m_updating) {
        report();
    }
}

//=============================================================================================================

std::map<std::string, double> ArtifactPanel::thresholds() const
{
    std::map<std::string, double> result;
    result["Active"] = rejection_active() ? 1.0 : 0.0;
    for (const std::string type : kChannelTypes) {
        if (form().get<bool>(present_key(type))) {
            result[type] = form().get<double>(mantissa_key(type)) * std::pow(10.0, form().get<int>(exponent_key(type)));
        }
    }
    return result;
}

//=============================================================================================================

bool ArtifactPanel::rejection_active() const
{
    return form().get<bool>(kHasTypes) && form().get<bool>(kActive);
}

//=============================================================================================================

void ArtifactPanel::load(const SettingsStore& store, const std::string& path)
{
    const bool updating = std::exchange(m_updating, true);
    Panel::load(store, path);
    m_updating = updating;
}

//=============================================================================================================

void ArtifactPanel::report()
{
    if (m_callback) {
        m_callback(thresholds());
    }
}
