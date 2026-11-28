//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_settings_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterSettingsPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_settings_panel.h"

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

constexpr const char* kClinical = "clinical";

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterSettingsPanel::FilterSettingsPanel()
: Panel("FilterSettingsView", "Filter Settings")
, m_design(std::make_shared<FilterDesignPanel>())
{
    SettingsForm& f = form();
    f.add_flag(kClinical);
    f.add_bool(active_key, "Activate filter", false);
    f.add_choice(channel_type_key, "Channel type", {{"All", "All"}, {"MEG", "MEG"}, {"EEG", "EEG"}, {"ECG", "ECG"}, {"EOG", "EOG"}, {"EMG", "EMG"}}, "All");
    for (const auto& [key, label] : {std::pair{from_key, "From"}, std::pair{to_key, "To"}}) {
        SettingField frequency;
        frequency.key = key;
        frequency.kind = SettingField::Kind::Double;
        frequency.label = label;
        frequency.default_value = 0.0;
        frequency.maximum = 10000000.0;
        frequency.step = 0.1;
        frequency.decimals = 1;
        frequency.unit = " Hz";
        frequency.commit = SettingField::Commit::WhenFinished;
        f.add(std::move(frequency));
    }
    SettingField advanced;
    advanced.key = show_design_key;
    advanced.kind = SettingField::Kind::Action;
    advanced.label = "Advanced filter design";
    advanced.visible_if = std::string("!") + kClinical;
    f.add(std::move(advanced));

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == show_design_key)) {
            changed(key);
        }
    });
    // The design panel's edges and channel type show here, whoever changed them
    m_designListener = m_design->form().add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (value && (key == FilterDesignPanel::from_key || key == FilterDesignPanel::to_key || key == FilterDesignPanel::channel_type_key)) {
            const bool updating = std::exchange(m_updating, true);
            form().set_value(key, *value);
            m_updating = updating;
        }
    });
    show_design_state();
}

//=============================================================================================================

FilterSettingsPanel::~FilterSettingsPanel()
{
    m_design->form().remove_listener(m_designListener);
    form().remove_listener(m_listener);
}

//=============================================================================================================

bool FilterSettingsPanel::active() const
{
    return form().get<bool>(active_key);
}

//=============================================================================================================

void FilterSettingsPanel::set_sampling_rate(double sfreq)
{
    m_design->set_sampling_rate(sfreq);
    show_design_state();
}

//=============================================================================================================

void FilterSettingsPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    const bool updating = std::exchange(m_updating, true);
    m_design->load(store, path);
    Panel::load(store, path);
    m_updating = updating;
    show_design_state();
}

//=============================================================================================================

void FilterSettingsPanel::save(SettingsStore& store, const std::string& path) const
{
    Panel::save(store, path);
    m_design->save(store, path);
}

//=============================================================================================================

void FilterSettingsPanel::gui_mode_changed()
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kClinical, gui_mode() == GuiMode::Clinical);
    m_updating = updating;
}

//=============================================================================================================

void FilterSettingsPanel::changed(const std::string& key)
{
    SettingsForm& design = m_design->form();
    if (key == show_design_key) {
        if (m_callbacks.show_design) {
            m_callbacks.show_design();
        }
    } else if (key == active_key) {
        if (m_callbacks.activation_changed) {
            m_callbacks.activation_changed(active());
        }
    } else if (key == from_key) {
        design.set_value(FilterDesignPanel::from_key, form().get<double>(key));
    } else if (key == to_key) {
        const double to = form().get<double>(key);
        {
            // The lower edge stays below the upper one
            const bool updating = std::exchange(m_updating, true);
            form().set_range(from_key, 0.0, form().get<double>(from_key) >= 2.0 ? to - 1.0 : to);
            m_updating = updating;
        }
        design.set_value(FilterDesignPanel::to_key, to);
    } else if (key == channel_type_key) {
        design.set_value(FilterDesignPanel::channel_type_key, form().get<std::string>(key));
    }
}

//=============================================================================================================

void FilterSettingsPanel::show_design_state()
{
    const bool updating = std::exchange(m_updating, true);
    const double nyquist = m_design->sampling_rate() / 2.0;
    form().set_range(from_key, 0.0, nyquist);
    form().set_range(to_key, 0.0, nyquist);
    form().set_value(to_key, m_design->to());
    form().set_value(from_key, m_design->from());
    form().set_value(channel_type_key, m_design->channel_type());
    m_updating = updating;
}
