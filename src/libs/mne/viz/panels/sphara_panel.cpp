//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sphara_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpharaPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sphara_panel.h"

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

constexpr const char* kActive = "activateSphara";
constexpr const char* kSystem = "system";
constexpr const char* kHasSecond = "hasSecond";
constexpr const char* kFirst = "first";
constexpr const char* kSecond = "second";

SettingField functions(const char* key, const char* label, int value)
{
    SettingField field;
    field.key = key;
    field.kind = SettingField::Kind::Int;
    field.label = label;
    field.default_value = value;
    field.minimum = 1;
    field.maximum = value;
    field.commit = SettingField::Commit::WhenFinished;
    field.persistent = false;
    return field;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpharaPanel::SpharaPanel()
: Panel("SpharaSettingsView", "SPHARA Settings")
{
    set_width(330);
    SettingsForm& f = form();
    SettingField activeField;
    activeField.key = kActive;
    activeField.kind = SettingField::Kind::Bool;
    activeField.label = "Activate SPHARA";
    activeField.default_value = false;
    activeField.persistent = false;
    f.add(std::move(activeField));
    SettingField systemField;
    systemField.key = kSystem;
    systemField.kind = SettingField::Kind::Choice;
    systemField.label = "System:";
    systemField.choices = {{"BabyMEG", "BabyMEG"}, {"VectorView", "VectorView"}, {"EEG", "EEG"}};
    systemField.default_value = std::string("BabyMEG");
    systemField.persistent = false;
    f.add(std::move(systemField));
    f.add_flag(kHasSecond, true);
    f.add(functions(kFirst, "Inner layer:", 270));
    SettingField secondField = functions(kSecond, "Outer layer:", 105);
    secondField.visible_if = kHasSecond;
    f.add(std::move(secondField));

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (m_updating || !value) {
            return;
        }
        if (key == kActive) {
            if (m_callbacks.activation_changed) {
                m_callbacks.activation_changed(active());
            }
        } else if (key == kSystem || key == kFirst || key == kSecond) {
            update_system_fields();
            if (m_callbacks.options_changed) {
                m_callbacks.options_changed(system(), first(), second());
            }
        }
    });
}

//=============================================================================================================

SpharaPanel::~SpharaPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

bool SpharaPanel::active() const
{
    return form().get<bool>(kActive);
}

//=============================================================================================================

std::string SpharaPanel::system() const
{
    return form().get<std::string>(kSystem);
}

//=============================================================================================================

int SpharaPanel::first() const
{
    return form().get<int>(kFirst);
}

//=============================================================================================================

int SpharaPanel::second() const
{
    return form().get<int>(kSecond);
}

//=============================================================================================================

void SpharaPanel::update_system_fields()
{
    const bool updating = std::exchange(m_updating, true);
    SettingsForm& f = form();
    const std::string current = system();
    f.set_value(kHasSecond, current != "EEG");
    if (current == "VectorView") {
        f.set_label(kFirst, "Mag");
        f.set_range(kFirst, 1, 102);
        f.set_label(kSecond, "Grad");
        f.set_range(kSecond, 1, 102);
    } else if (current == "BabyMEG") {
        f.set_label(kFirst, "Inner layer");
        f.set_range(kFirst, 1, 270);
        f.set_label(kSecond, "Outer layer");
        f.set_range(kSecond, 1, 105);
    } else if (current == "EEG") {
        f.set_label(kFirst, "EEG");
        f.set_range(kFirst, 1, 256);
    }
    m_updating = updating;
}
