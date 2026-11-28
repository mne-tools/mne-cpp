//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     connectivity_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ConnectivityPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <initializer_list>
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

constexpr const char* kMetric = "connMethod";
constexpr const char* kWindow = "connWindowType";
constexpr const char* kTrials = "connNrTrials";
constexpr const char* kTrigger = "connTriggerType";
constexpr const char* kLow = "connFreqLow";
constexpr const char* kHigh = "connFreqHigh";

std::vector<SettingChoice> plain_choices(std::initializer_list<const char*> values)
{
    std::vector<SettingChoice> choices;
    choices.reserve(values.size());
    for (const char* value : values) {
        choices.push_back({value, value});
    }
    return choices;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivityPanel::ConnectivityPanel()
: Panel("ConnectivitySettingsView", "Connectivity Settings")
{
    set_width(330);
    SettingsForm& f = form();
    f.add_choice(kMetric, "Metric:", plain_choices({"COR", "COH", "IMAGCOH", "XCOR", "PLI", "DSWPLI", "USPLI", "WPLI", "PLV"}), "COR");
    f.add_choice(kWindow, "Window Type:", plain_choices({"Hanning", "Ones"}), "Hanning");
    SettingField trials;
    trials.key = kTrials;
    trials.kind = SettingField::Kind::Int;
    trials.label = "Number of trials:";
    trials.default_value = 10;
    trials.minimum = 1;
    trials.maximum = 1000;
    trials.decimals = 0;
    trials.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(trials));
    f.add_choice(kTrigger, "Trigger type:", {}, {});
    f.add_double(kLow, "Lower frequency:", 7.0, 0.0, 10000.0, 1.0, 1);
    f.add_double(kHigh, "Upper frequency:", 13.0, 0.0, 10000.0, 1.0, 1);

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && value) {
            changed(key);
        }
    });
}

//=============================================================================================================

ConnectivityPanel::~ConnectivityPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void ConnectivityPanel::set_trigger_types(const std::vector<std::string>& types)
{
    std::vector<SettingChoice> choices = form().field(kTrigger)->choices;
    const std::size_t count = choices.size();
    for (const std::string& type : types) {
        if (std::ranges::none_of(choices, [&type](const SettingChoice& choice) { return choice.value == type; })) {
            choices.push_back({type, type});
        }
    }
    if (choices.size() != count) {
        form().set_choices(kTrigger, std::move(choices));
    }
}

//=============================================================================================================

void ConnectivityPanel::set_number_trials(int trials)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kTrials, trials);
    m_trials = number_trials();
    m_updating = updating;
}

//=============================================================================================================

void ConnectivityPanel::set_frequency_band(double low, double high)
{
    form().set_value(kLow, low);
    form().set_value(kHigh, high);
}

//=============================================================================================================

std::string ConnectivityPanel::metric() const
{
    return form().get<std::string>(kMetric);
}

//=============================================================================================================

std::string ConnectivityPanel::window_type() const
{
    return form().get<std::string>(kWindow);
}

//=============================================================================================================

int ConnectivityPanel::number_trials() const
{
    return form().get<int>(kTrials);
}

//=============================================================================================================

std::string ConnectivityPanel::trigger_type() const
{
    return form().get<std::string>(kTrigger);
}

//=============================================================================================================

double ConnectivityPanel::lower_frequency() const
{
    return form().get<double>(kLow);
}

//=============================================================================================================

double ConnectivityPanel::upper_frequency() const
{
    return form().get<double>(kHigh);
}

//=============================================================================================================

void ConnectivityPanel::load(const SettingsStore& store, const std::string& path)
{
    const bool updating = std::exchange(m_updating, true);
    Panel::load(store, path);
    m_trials = number_trials();
    m_updating = updating;
}

//=============================================================================================================

void ConnectivityPanel::changed(const std::string& key)
{
    if (key == kMetric) {
        if (m_callbacks.metric_changed) {
            m_callbacks.metric_changed(metric());
        }
    } else if (key == kWindow) {
        if (m_callbacks.window_type_changed) {
            m_callbacks.window_type_changed(window_type());
        }
    } else if (key == kTrials) {
        if (number_trials() != m_trials) {
            m_trials = number_trials();
            if (m_callbacks.number_trials_changed) {
                m_callbacks.number_trials_changed(m_trials);
            }
        }
    } else if (key == kTrigger) {
        if (m_callbacks.trigger_type_changed) {
            m_callbacks.trigger_type_changed(trigger_type());
        }
    } else if (key == kLow || key == kHigh) {
        if (m_callbacks.frequency_band_changed) {
            m_callbacks.frequency_band_changed(static_cast<float>(lower_frequency()), static_cast<float>(upper_frequency()));
        }
    }
}
