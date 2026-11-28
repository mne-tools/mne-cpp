//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     covariance_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CovariancePanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance_panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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

constexpr const char* kSamples = "samples";
constexpr int kMinimumSamples = 600;
constexpr int kSecondsMost = 60;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovariancePanel::CovariancePanel()
: Panel("CovarianceSettingsView", "Covariance Settings")
{
    set_width(330);
    SettingField samples;
    samples.key = kSamples;
    samples.kind = SettingField::Kind::Int;
    samples.label = "Number of Samples";
    samples.default_value = kMinimumSamples;
    samples.minimum = kMinimumSamples;
    samples.maximum = kMinimumSamples * kSecondsMost;
    samples.step = kMinimumSamples;
    samples.persistent = false;
    form().add(std::move(samples));
    m_listener = form().add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && value && key == kSamples && m_callback) {
            m_callback(this->samples());
        }
    });
}

//=============================================================================================================

CovariancePanel::~CovariancePanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void CovariancePanel::set_minimum_samples(int samples)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_range(kSamples, samples, static_cast<double>(samples) * kSecondsMost);
    m_updating = updating;
}

//=============================================================================================================

void CovariancePanel::set_samples(int samples)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kSamples, samples);
    m_updating = updating;
}

//=============================================================================================================

int CovariancePanel::samples() const
{
    return form().get<int>(kSamples);
}
