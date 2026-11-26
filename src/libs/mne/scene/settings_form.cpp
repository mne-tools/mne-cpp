//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settings_form.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsForm definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settings_form.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsForm::SettingsForm(std::string title)
: m_title(std::move(title))
{
}

//=============================================================================================================

SettingField* SettingsForm::find(const std::string& key)
{
    const auto it = std::find_if(m_fields.begin(), m_fields.end(), [&key](const SettingField& f) {
        return f.key == key;
    });
    return it == m_fields.end() ? nullptr : &*it;
}

//=============================================================================================================

const SettingField* SettingsForm::field(const std::string& key) const
{
    const auto it = std::find_if(m_fields.begin(), m_fields.end(), [&key](const SettingField& f) {
        return f.key == key;
    });
    return it == m_fields.end() ? nullptr : &*it;
}

//=============================================================================================================

const SettingField& SettingsForm::add(SettingField field)
{
    if (field.maximum < field.minimum) {
        field.maximum = field.minimum;
    }
    const std::string key = field.key;
    SettingField* stored = find(key);
    if (stored) {
        *stored = std::move(field);
    } else {
        m_fields.push_back(std::move(field));
        stored = &m_fields.back();
    }
    m_values.erase(key);
    if (stored->kind != SettingField::Kind::Action && stored->kind != SettingField::Kind::Custom) {
        SettingValue initial = stored->default_value;
        if (stored->kind == SettingField::Kind::Choice && !stored->choices.empty() && !std::holds_alternative<std::string>(initial)) {
            initial = stored->choices.front().value;
        }
        const std::optional<SettingValue> valid = valid_value(*stored, initial);
        if (valid) {
            m_values[key] = *valid;
        } else if (stored->kind == SettingField::Kind::Choice && !stored->choices.empty()) {
            m_values[key] = stored->choices.front().value;
        }
    }
    notify_structure(key);
    return *find(key);
}

//=============================================================================================================

const SettingField& SettingsForm::add_bool(const std::string& key, const std::string& label, bool value, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Bool;
    f.label = label;
    f.default_value = value;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_int(const std::string& key, const std::string& label, int value, int minimum, int maximum, int step,
                                          const std::string& unit, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Int;
    f.label = label;
    f.default_value = value;
    f.minimum = minimum;
    f.maximum = maximum;
    f.step = step;
    f.decimals = 0;
    f.unit = unit;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_double(const std::string& key, const std::string& label, double value, double minimum, double maximum,
                                             double step, int decimals, const std::string& unit, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Double;
    f.label = label;
    f.default_value = value;
    f.minimum = minimum;
    f.maximum = maximum;
    f.step = step;
    f.decimals = decimals;
    f.unit = unit;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_choice(const std::string& key, const std::string& label, std::vector<SettingChoice> choices,
                                             const std::string& value, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Choice;
    f.label = label;
    f.choices = std::move(choices);
    f.default_value = value;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_text(const std::string& key, const std::string& label, const std::string& value, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Text;
    f.label = label;
    f.default_value = value;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_action(const std::string& key, const std::string& label, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Action;
    f.label = label;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

const SettingField& SettingsForm::add_custom(const std::string& key, const std::string& label, const std::string& group)
{
    SettingField f;
    f.key = key;
    f.kind = SettingField::Kind::Custom;
    f.label = label;
    f.group = group;
    return add(std::move(f));
}

//=============================================================================================================

std::vector<std::string> SettingsForm::groups() const
{
    std::vector<std::string> result;
    for (const SettingField& f : m_fields) {
        if (std::find(result.begin(), result.end(), f.group) == result.end()) {
            result.push_back(f.group);
        }
    }
    return result;
}

//=============================================================================================================

std::optional<SettingValue> SettingsForm::valid_value(const SettingField& field, const SettingValue& value) const
{
    switch (field.kind) {
        case SettingField::Kind::Bool:
            if (const bool* on = std::get_if<bool>(&value)) {
                return *on;
            }
            return std::nullopt;
        case SettingField::Kind::Int:
        case SettingField::Kind::Double: {
            double number = 0.0;
            if (const int* i = std::get_if<int>(&value)) {
                number = *i;
            } else if (const double* d = std::get_if<double>(&value)) {
                number = *d;
            } else {
                return std::nullopt;
            }
            if (!std::isfinite(number)) {
                return std::nullopt;
            }
            number = std::clamp(number, field.minimum, field.maximum);
            if (field.kind == SettingField::Kind::Int) {
                return static_cast<int>(std::lround(number));
            }
            return number;
        }
        case SettingField::Kind::Choice: {
            const std::string* choice = std::get_if<std::string>(&value);
            if (!choice || std::none_of(field.choices.begin(), field.choices.end(), [choice](const SettingChoice& c) {
                    return c.value == *choice;
                })) {
                return std::nullopt;
            }
            return *choice;
        }
        case SettingField::Kind::Text:
        case SettingField::Kind::Path:
            if (const std::string* text = std::get_if<std::string>(&value)) {
                return *text;
            }
            return std::nullopt;
        case SettingField::Kind::Action:
        case SettingField::Kind::Custom:
            break;
    }
    return std::nullopt;
}

//=============================================================================================================

std::optional<SettingValue> SettingsForm::value(const std::string& key) const
{
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return std::nullopt;
    }
    return it->second;
}

//=============================================================================================================

void SettingsForm::store(const std::string& key, const SettingValue& value)
{
    const auto it = m_values.find(key);
    if (it != m_values.end() && it->second == value) {
        return;
    }
    m_values[key] = value;
    // Listeners may add or remove listeners
    const std::map<int, Listener> listeners = m_listeners;
    for (const auto& [id, listener] : listeners) {
        listener(key, value);
    }
}

//=============================================================================================================

bool SettingsForm::set_value(const std::string& key, const SettingValue& value)
{
    const SettingField* f = field(key);
    if (!f) {
        return false;
    }
    const std::optional<SettingValue> valid = valid_value(*f, value);
    if (!valid) {
        return false;
    }
    const auto it = m_values.find(key);
    if (it != m_values.end() && it->second == *valid) {
        return false;
    }
    store(key, *valid);
    return true;
}

//=============================================================================================================

void SettingsForm::set_values(const std::map<std::string, SettingValue>& values)
{
    for (const auto& [key, value] : values) {
        set_value(key, value);
    }
}

//=============================================================================================================

void SettingsForm::reset()
{
    for (const SettingField& f : m_fields) {
        set_value(f.key, f.default_value);
    }
}

//=============================================================================================================

void SettingsForm::set_range(const std::string& key, double minimum, double maximum)
{
    SettingField* f = find(key);
    if (!f || (f->kind != SettingField::Kind::Int && f->kind != SettingField::Kind::Double)) {
        return;
    }
    f->minimum = minimum;
    f->maximum = std::max(maximum, minimum);
    notify_structure(key);
    if (const auto it = m_values.find(key); it != m_values.end()) {
        if (const std::optional<SettingValue> valid = valid_value(*f, it->second)) {
            store(key, *valid);
        }
    }
}

//=============================================================================================================

void SettingsForm::set_choices(const std::string& key, std::vector<SettingChoice> choices)
{
    SettingField* f = find(key);
    if (!f || f->kind != SettingField::Kind::Choice) {
        return;
    }
    f->choices = std::move(choices);
    notify_structure(key);
    const auto it = m_values.find(key);
    if (it == m_values.end() || !valid_value(*f, it->second)) {
        if (f->choices.empty()) {
            m_values.erase(key);
        } else {
            store(key, f->choices.front().value);
        }
    }
}

//=============================================================================================================

void SettingsForm::set_label(const std::string& key, const std::string& label)
{
    if (SettingField* f = find(key); f && f->label != label) {
        f->label = label;
        notify_structure(key);
    }
}

//=============================================================================================================

void SettingsForm::trigger(const std::string& key)
{
    const SettingField* f = field(key);
    if (!f || f->kind != SettingField::Kind::Action) {
        return;
    }
    const std::map<int, Listener> listeners = m_listeners;
    for (const auto& [id, listener] : listeners) {
        listener(key, std::nullopt);
    }
}

//=============================================================================================================

bool SettingsForm::condition_holds(const std::string& condition) const
{
    if (condition.empty()) {
        return true;
    }
    const bool negated = condition.front() == '!';
    const bool on = get<bool>(negated ? condition.substr(1) : condition, false);
    return negated ? !on : on;
}

//=============================================================================================================

bool SettingsForm::is_enabled(const std::string& key) const
{
    const SettingField* f = field(key);
    return f && condition_holds(f->enabled_if);
}

//=============================================================================================================

bool SettingsForm::is_visible(const std::string& key) const
{
    const SettingField* f = field(key);
    return f && condition_holds(f->visible_if);
}

//=============================================================================================================

int SettingsForm::add_listener(Listener listener)
{
    const int id = m_nextListener++;
    m_listeners[id] = std::move(listener);
    return id;
}

//=============================================================================================================

int SettingsForm::add_structure_listener(std::function<void(const std::string& key)> listener)
{
    const int id = m_nextListener++;
    m_structureListeners[id] = std::move(listener);
    return id;
}

//=============================================================================================================

void SettingsForm::remove_listener(int id)
{
    m_listeners.erase(id);
    m_structureListeners.erase(id);
}

//=============================================================================================================

void SettingsForm::notify_structure(const std::string& key)
{
    const auto listeners = m_structureListeners;
    for (const auto& [id, listener] : listeners) {
        listener(key);
    }
}
