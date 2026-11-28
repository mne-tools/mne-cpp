//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trigger_detection_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TriggerDetectionPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trigger_detection_panel.h"
#include "../host/settings_store.h"
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
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

constexpr const char* kRealTime = "realTime";
constexpr const char* kActive = "activateTriggerDetection";
constexpr const char* kChannel = "triggerChannel";
constexpr const char* kColorType = "triggerColorType";
constexpr const char* kColor = "triggerColor";
constexpr const char* kMantissa = "detectionThresholdFirst";
constexpr const char* kExponent = "detectionThresholdSecond";
constexpr const char* kDetected = "numberDetectedTriggers";
constexpr const char* kReset = "resetNumberTriggers";
constexpr const char* kDetect = "detectTriggers";
constexpr const char* kDefaultColor = "#b10000";

/** A trigger type as text, in the general notation with six significant digits. */
std::string type_text(double type)
{
    std::ostringstream text;
    text << type;
    return text.str();
}

/** A trigger type from its text; text that is no number is type 0. */
double type_value(const std::string& text)
{
    std::istringstream stream(text);
    double value = 0.0;
    stream >> value;
    return stream.fail() ? 0.0 : value;
}

/** A field shown in real-time mode only. */
SettingField real_time_field(const char* key, SettingField::Kind kind, const std::string& label)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.visible_if = kRealTime;
    field.persistent = false;
    return field;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerDetectionPanel::TriggerDetectionPanel()
: Panel("TriggerDetectionView", "Trigger Detection Settings")
{
    set_width(330);
    SettingsForm& f = form();
    f.add_flag(kRealTime, true);
    SettingField activeField = real_time_field(kActive, SettingField::Kind::Bool, "Activate trigger detection");
    activeField.default_value = false;
    f.add(std::move(activeField));
    SettingField channel;
    channel.key = kChannel;
    channel.kind = SettingField::Kind::Choice;
    channel.label = "Trigger channel:";
    channel.persistent = false;
    f.add(std::move(channel));
    f.add(real_time_field(kColorType, SettingField::Kind::Choice, "Type color:"));
    SettingField color = real_time_field(kColor, SettingField::Kind::Color, {});
    color.default_value = std::string(kDefaultColor);
    f.add(std::move(color));
    SettingField mantissa;
    mantissa.key = kMantissa;
    mantissa.kind = SettingField::Kind::Double;
    mantissa.label = "Threshold:";
    mantissa.default_value = 0.1;
    mantissa.minimum = -1000.0;
    mantissa.maximum = 1000.0;
    mantissa.step = 0.1;
    mantissa.decimals = 2;
    mantissa.persistent = false;
    f.add(std::move(mantissa));
    SettingField exponent;
    exponent.key = kExponent;
    exponent.kind = SettingField::Kind::Int;
    exponent.default_value = -1;
    exponent.minimum = -100;
    exponent.maximum = 100;
    exponent.decimals = 0;
    exponent.prefix = "e^";
    exponent.persistent = false;
    f.add(std::move(exponent));
    SettingField detected = real_time_field(kDetected, SettingField::Kind::Info, "Detected trigger:");
    detected.default_value = std::string("0");
    f.add(std::move(detected));
    f.add(real_time_field(kReset, SettingField::Kind::Action, "Reset"));
    SettingField detect;
    detect.key = kDetect;
    detect.kind = SettingField::Kind::Action;
    detect.label = "Detect Triggers";
    detect.visible_if = "!realTime";
    f.add(std::move(detect));

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>&) {
        if (!m_updating && m_initialized) {
            changed(key);
        }
    });
}

//=============================================================================================================

TriggerDetectionPanel::~TriggerDetectionPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void TriggerDetectionPanel::init(const mne::FiffInfo& info)
{
    std::vector<SettingChoice> channels;
    for (const auto& ch : info.chs) {
        if (ch.kind == FIFFV_STIM_CH) {
            channels.push_back({ch.ch_name, ch.ch_name});
        }
    }
    {
        const bool updating = std::exchange(m_updating, true);
        if (m_storedChannel >= 0 && m_storedChannel < static_cast<int>(channels.size())) {
            const std::string stored = channels[static_cast<std::size_t>(m_storedChannel)].value;
            form().set_choices(kChannel, std::move(channels));
            form().set_value(kChannel, stored);
        } else {
            form().set_choices(kChannel, std::move(channels));
        }
        m_updating = updating;
    }
    m_initialized = true;
    show_type_color();
    report_info();
}

//=============================================================================================================

void TriggerDetectionPanel::set_detected_triggers(int total, const std::map<int, std::vector<std::pair<int, double>>>& triggers)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kDetected, std::to_string(total));
    std::vector<SettingChoice> types = form().field(kColorType)->choices;
    const std::size_t count = types.size();
    for (const auto& [channel, detections] : triggers) {
        for (const auto& [sample, value] : detections) {
            const std::string type = type_text(value);
            if (std::ranges::none_of(types, [&type](const SettingChoice& choice) { return choice.value == type; })) {
                types.push_back({type, type});
            }
        }
    }
    if (types.size() != count) {
        form().set_choices(kColorType, std::move(types));
    }
    m_updating = updating;
    show_type_color();
}

//=============================================================================================================

std::string TriggerDetectionPanel::selected_stim_channel() const
{
    return form().get<std::string>(kChannel);
}

//=============================================================================================================

double TriggerDetectionPanel::threshold() const
{
    return form().get<double>(kMantissa) * std::pow(10.0, form().get<int>(kExponent));
}

//=============================================================================================================

bool TriggerDetectionPanel::active() const
{
    return form().get<bool>(kActive);
}

//=============================================================================================================

void TriggerDetectionPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kActive, store.read_bool(settings_key(path, "Activated"), false));
    m_storedChannel = store.read_int(settings_key(path, "ChannelIndex"), 0);
    const std::vector<SettingChoice>& channels = form().field(kChannel)->choices;
    if (m_storedChannel >= 0 && m_storedChannel < static_cast<int>(channels.size())) {
        form().set_value(kChannel, channels[static_cast<std::size_t>(m_storedChannel)].value);
    }
    form().set_value(kMantissa, store.read_double(settings_key(path, "FirstThresholdValue"), 0.1));
    form().set_value(kExponent, store.read_int(settings_key(path, "SecondThresholdValue"), -1));
    std::istringstream types(store.read_string(settings_key(path, "ColorTypes"), {}));
    for (std::string type; std::getline(types, type, ';');) {
        if (!type.empty()) {
            m_colors[type_value(type)] = store.read_string(settings_key(path, "Colors/" + type), kDefaultColor);
        }
    }
    m_updating = updating;
    if (m_initialized) {
        show_type_color();
        report_info();
    }
}

//=============================================================================================================

void TriggerDetectionPanel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    const std::vector<SettingChoice>& channels = form().field(kChannel)->choices;
    const auto current = std::ranges::find(channels, selected_stim_channel(), &SettingChoice::value);
    const int index = current == channels.end() ? -1 : static_cast<int>(current - channels.begin());
    store.set_value(settings_key(path, "Activated"), active());
    store.set_value(settings_key(path, "ChannelIndex"), index);
    store.set_value(settings_key(path, "FirstThresholdValue"), form().get<double>(kMantissa));
    store.set_value(settings_key(path, "SecondThresholdValue"), form().get<int>(kExponent));
    std::string types;
    for (const auto& [type, color] : m_colors) {
        const std::string text = type_text(type);
        store.set_value(settings_key(path, "Colors/" + text), color);
        types += (types.empty() ? "" : ";") + text;
    }
    store.set_value(settings_key(path, "ColorTypes"), types);
}

//=============================================================================================================

void TriggerDetectionPanel::processing_mode_changed()
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kRealTime, processing_mode() != ProcessingMode::Offline);
    m_updating = updating;
}

//=============================================================================================================

void TriggerDetectionPanel::changed(const std::string& key)
{
    if (key == kActive || key == kChannel || key == kMantissa || key == kExponent) {
        report_info();
    } else if (key == kColorType) {
        show_type_color();
    } else if (key == kColor) {
        m_colors[type_value(form().get<std::string>(kColorType))] = form().get<std::string>(kColor);
        report_info();
    } else if (key == kReset) {
        const bool updating = std::exchange(m_updating, true);
        form().set_value(kDetected, std::string("0"));
        form().set_choices(kColorType, {});
        m_updating = updating;
        show_type_color();
        if (m_callbacks.reset_trigger_counter) {
            m_callbacks.reset_trigger_counter();
        }
    } else if (key == kDetect) {
        if (!selected_stim_channel().empty() && m_callbacks.detect_triggers) {
            m_callbacks.detect_triggers(selected_stim_channel(), threshold());
        }
    }
}

//=============================================================================================================

void TriggerDetectionPanel::report_info()
{
    if (m_callbacks.trigger_info_changed) {
        m_callbacks.trigger_info_changed(m_colors, active(), selected_stim_channel(), threshold());
    }
}

//=============================================================================================================

void TriggerDetectionPanel::show_type_color()
{
    const auto it = m_colors.find(type_value(form().get<std::string>(kColorType)));
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kColor, it == m_colors.end() ? std::string(kDefaultColor) : it->second);
    m_updating = updating;
}
