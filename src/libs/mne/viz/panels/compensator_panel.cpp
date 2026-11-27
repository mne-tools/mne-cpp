//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     compensator_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CompensatorPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compensator_panel.h"
#include "../host/settings_store.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <string_view>
#include <system_error>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

constexpr int kPanelWidth = 330;
constexpr const char* kFieldPrefix = "comp_";

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CompensatorPanel::CompensatorPanel()
: Panel("CompensatorView", "Compensators")
{
    set_width(kPanelWidth);
    m_listener = form().add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (m_updating || !value || !key.starts_with(kFieldPrefix)) {
            return;
        }
        int kind = 0;
        const char* first = key.data() + std::char_traits<char>::length(kFieldPrefix);
        std::from_chars(first, key.data() + key.size(), kind);
        if (const bool* on = std::get_if<bool>(&*value)) {
            toggled(kind, *on);
        }
    });
}

//=============================================================================================================

CompensatorPanel::~CompensatorPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

std::string CompensatorPanel::field_key(int kind)
{
    return kFieldPrefix + std::to_string(kind);
}

//=============================================================================================================

void CompensatorPanel::set_compensators(const std::vector<mne::FiffCtfComp>& comps)
{
    for (const mne::FiffCtfComp& comp : m_comps) {
        form().remove(field_key(comp.kind));
    }
    m_comps = comps;
    m_selected = 0;
    int restored = 0;
    m_updating = true;
    for (const mne::FiffCtfComp& comp : m_comps) {
        const bool on = m_active.try_emplace(comp.kind, false).first->second;
        SettingField field;
        field.key = field_key(comp.kind);
        field.kind = SettingField::Kind::Bool;
        field.label = std::to_string(comp.kind);
        field.default_value = false;
        field.persistent = false;
        form().add(std::move(field));
        if (on && restored == 0) {
            restored = comp.kind;
        }
    }
    m_updating = false;
    // A remembered grade is chosen again
    if (restored != 0) {
        select(restored);
    }
}

//=============================================================================================================

void CompensatorPanel::select(int to)
{
    m_updating = true;
    for (const mne::FiffCtfComp& comp : m_comps) {
        const bool on = comp.kind == to;
        form().set_value(field_key(comp.kind), on);
        m_active[comp.kind] = on;
    }
    m_updating = false;
    const bool shown = std::any_of(m_comps.begin(), m_comps.end(), [to](const mne::FiffCtfComp& comp) {
        return comp.kind == to;
    });
    m_selected = shown ? to : 0;
    if (m_callback) {
        m_callback(m_selected);
    }
}

//=============================================================================================================

void CompensatorPanel::toggled(int kind, bool on)
{
    select(on ? kind : 0);
}

//=============================================================================================================

void CompensatorPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    // "kind:on" pairs separated by commas, e.g. "1:0,3:1"
    const std::string stored = store.read_string(settings_key(path, "active"), {});
    std::size_t begin = 0;
    while (begin < stored.size()) {
        const std::size_t end = std::min(stored.find(',', begin), stored.size());
        const std::string_view entry(stored.data() + begin, end - begin);
        const std::size_t colon = entry.find(':');
        int kind = 0;
        if (colon != std::string_view::npos && std::from_chars(entry.data(), entry.data() + colon, kind).ec == std::errc()) {
            m_active[kind] = entry.substr(colon + 1) == "1";
        }
        begin = end + 1;
    }
}

//=============================================================================================================

void CompensatorPanel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    std::string stored;
    for (const auto& [kind, on] : m_active) {
        stored += (stored.empty() ? "" : ",") + std::to_string(kind) + (on ? ":1" : ":0");
    }
    store.set_value(settings_key(path, "active"), stored);
}
