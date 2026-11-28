//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     minimum_norm_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    MinimumNormPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "minimum_norm_panel.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
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

constexpr const char* kMethodEditable = "methodEditable";
constexpr const char* kIsCmne = "isCmne";
constexpr const char* kMethod = "method";
constexpr const char* kTriggerType = "triggerType";
constexpr const char* kTimePoint = "timePoint";
constexpr const char* kCheckpoint = "modelCheckpoint";

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimumNormPanel::MinimumNormPanel(const std::string& method)
: Panel("MinimumNormSettingsView", "InvMinimumNorm Settings")
{
    set_width(330);
    SettingsForm& f = form();
    f.add_flag(kMethodEditable, true);
    f.add_flag(kIsCmne);

    SettingField choice;
    choice.key = kMethod;
    choice.kind = SettingField::Kind::Choice;
    choice.label = "Method:";
    choice.choices = {{"MNE", "MNE"}, {"dSPM", "dSPM"}, {"sLORETA", "sLORETA"}, {"CMNE", "CMNE"}};
    choice.default_value = std::string("MNE");
    choice.enabled_if = kMethodEditable;
    choice.persistent = false;
    f.add(std::move(choice));

    SettingField trigger;
    trigger.key = kTriggerType;
    trigger.kind = SettingField::Kind::Choice;
    trigger.label = "Trigger type:";
    trigger.default_value = std::string();
    trigger.persistent = false;
    f.add(std::move(trigger));

    SettingField timePoint;
    timePoint.key = kTimePoint;
    timePoint.kind = SettingField::Kind::Int;
    timePoint.label = "Time point (incl. pre stim):";
    timePoint.default_value = 1;
    timePoint.minimum = 1;
    timePoint.maximum = 10000;
    timePoint.decimals = 0;
    timePoint.unit = "ms";
    timePoint.commit = SettingField::Commit::WhenFinished;
    timePoint.persistent = false;
    f.add(std::move(timePoint));

    SettingField checkpoint;
    checkpoint.key = kCheckpoint;
    checkpoint.kind = SettingField::Kind::Path;
    checkpoint.label = "Model checkpoint:";
    checkpoint.tooltip = "Path to .onnx LSTM checkpoint";
    checkpoint.default_value = std::string();
    checkpoint.file_filter = "ONNX model (*.onnx);;All files (*)";
    checkpoint.commit = SettingField::Commit::WhenFinished;
    checkpoint.visible_if = kIsCmne;
    f.add(std::move(checkpoint));

    if (!method.empty()) {
        f.set_value(kMethod, method);
        f.set_value(kMethodEditable, false);
    }
    f.set_value(kIsCmne, this->method() == "CMNE");

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!value) {
            return;
        }
        if (key == kMethod) {
            form().set_value(kIsCmne, this->method() == "CMNE");
            if (m_callbacks.method_changed) {
                m_callbacks.method_changed(this->method());
            }
        } else if (key == kTriggerType) {
            if (m_callbacks.trigger_type_changed) {
                m_callbacks.trigger_type_changed(form().get<std::string>(kTriggerType));
            }
        } else if (key == kTimePoint) {
            if (m_callbacks.time_point_changed) {
                m_callbacks.time_point_changed(form().get<int>(kTimePoint));
            }
        } else if (key == kCheckpoint) {
            set_model_checkpoint(mne::path_from_utf8(form().get<std::string>(kCheckpoint)));
        }
    });
}

//=============================================================================================================

MinimumNormPanel::~MinimumNormPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void MinimumNormPanel::set_trigger_types(const std::vector<std::string>& types)
{
    std::vector<SettingChoice> choices = form().field(kTriggerType)->choices;
    const std::size_t count = choices.size();
    for (const std::string& type : types) {
        if (std::ranges::none_of(choices, [&type](const SettingChoice& choice) { return choice.value == type; })) {
            choices.push_back({type, type});
        }
    }
    if (choices.size() != count) {
        form().set_choices(kTriggerType, std::move(choices));
    }
}

//=============================================================================================================

void MinimumNormPanel::set_model_checkpoint(const std::filesystem::path& path)
{
    if (m_checkpoint == path) {
        return;
    }
    m_checkpoint = path;
    form().set_value(kCheckpoint, mne::utf8_from_path(path));
    if (m_callbacks.model_checkpoint_changed) {
        m_callbacks.model_checkpoint_changed(path);
    }
}

//=============================================================================================================

std::filesystem::path MinimumNormPanel::model_checkpoint() const
{
    return m_checkpoint;
}

//=============================================================================================================

std::string MinimumNormPanel::method() const
{
    return form().get<std::string>(kMethod);
}
