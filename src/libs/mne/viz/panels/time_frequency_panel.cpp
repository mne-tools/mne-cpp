//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     time_frequency_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TimeFrequencyPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "time_frequency_panel.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyPanel::TimeFrequencyPanel()
: Panel("TfSettingsView", "Time frequency Settings")
{
    set_width(330);
    form().add_int("trialNumber", "Trial number", 0, 0, 10000);
    form().add_int("rowNumber", "Row number", 0, 0, 10000);
    m_listener = form().add_listener([this](const std::string&, const std::optional<SettingValue>& value) {
        if (value && m_callback) {
            m_callback(trial(), row());
        }
    });
}

//=============================================================================================================

TimeFrequencyPanel::~TimeFrequencyPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

int TimeFrequencyPanel::trial() const
{
    return form().get<int>("trialNumber");
}

//=============================================================================================================

int TimeFrequencyPanel::row() const
{
    return form().get<int>("rowNumber");
}
