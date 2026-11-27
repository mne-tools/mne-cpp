//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     projectors_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ProjectorsPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectors_panel.h"
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
constexpr const char* kFieldPrefix = "proj_";

// Descriptions may contain anything but the separators, which are escaped
std::string escape(const std::string& text)
{
    std::string out;
    for (const char c : text) {
        if (c == '\\' || c == '\n' || c == '=') {
            out += '\\';
        }
        out += c == '\n' ? 'n' : c;
    }
    return out;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectorsPanel::ProjectorsPanel()
: Panel("ProjectorsView", "Projectors")
{
    set_width(kPanelWidth);
    m_listener = form().add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        const bool* on = value ? std::get_if<bool>(&*value) : nullptr;
        if (m_updating || !on || !key.starts_with(kFieldPrefix)) {
            return;
        }
        if (key == kAllKey) {
            set_all_active(*on);
            return;
        }
        std::size_t index = 0;
        const char* first = key.data() + std::char_traits<char>::length(kFieldPrefix);
        if (std::from_chars(first, key.data() + key.size(), index).ec == std::errc()) {
            set_active(index, *on);
        }
    });
}

//=============================================================================================================

ProjectorsPanel::~ProjectorsPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

std::string ProjectorsPanel::field_key(std::size_t index)
{
    return kFieldPrefix + std::to_string(index);
}

//=============================================================================================================

void ProjectorsPanel::set_projectors(const std::vector<mne::FiffProj>& projs)
{
    m_updating = true;
    for (std::size_t i = 0; i < m_projs.size(); ++i) {
        form().remove(field_key(i));
    }
    form().remove(kAllKey);
    m_projs = projs;
    for (std::size_t i = 0; i < m_projs.size(); ++i) {
        mne::FiffProj& proj = m_projs[i];
        proj.active = m_active.try_emplace(proj.desc, proj.active).first->second;
        SettingField field;
        field.key = field_key(i);
        field.kind = SettingField::Kind::Bool;
        field.label = proj.desc;
        field.default_value = proj.active;
        field.persistent = false;
        form().add(std::move(field));
    }
    if (!m_projs.empty()) {
        SettingField all;
        all.key = kAllKey;
        all.kind = SettingField::Kind::Bool;
        all.label = "Enable all";
        all.default_value = false;
        all.persistent = false;
        form().add(std::move(all));
    }
    m_updating = false;
    changed();
}

//=============================================================================================================

void ProjectorsPanel::set_active(std::size_t index, bool active)
{
    if (index >= m_projs.size()) {
        return;
    }
    apply(index, active);
    changed();
}

//=============================================================================================================

void ProjectorsPanel::set_all_active(bool active)
{
    for (std::size_t i = 0; i < m_projs.size(); ++i) {
        apply(i, active);
    }
    changed();
}

//=============================================================================================================

void ProjectorsPanel::apply(std::size_t index, bool active)
{
    m_projs[index].active = active;
    m_active[m_projs[index].desc] = active;
    m_updating = true;
    form().set_value(field_key(index), active);
    m_updating = false;
}

//=============================================================================================================

void ProjectorsPanel::changed()
{
    if (m_projs.empty()) {
        return;
    }
    const bool all = std::all_of(m_projs.begin(), m_projs.end(), [](const mne::FiffProj& proj) {
        return proj.active;
    });
    m_updating = true;
    form().set_value(kAllKey, all);
    m_updating = false;
    if (m_callback) {
        m_callback(m_projs);
    }
}

//=============================================================================================================

void ProjectorsPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    // One "description=0|1" per line, with '\', '=' and line breaks escaped
    const std::string stored = store.read_string(settings_key(path, "active"), {});
    std::string desc;
    bool escaped = false;
    for (std::size_t i = 0; i < stored.size(); ++i) {
        const char c = stored[i];
        if (escaped) {
            desc += c == 'n' ? '\n' : c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '=' && i + 1 < stored.size()) {
            m_active[desc] = stored[i + 1] == '1';
            desc.clear();
            i = std::min(stored.find('\n', i), stored.size());
        } else {
            desc += c;
        }
    }
}

//=============================================================================================================

void ProjectorsPanel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    std::string stored;
    for (const auto& [desc, on] : m_active) {
        stored += escape(desc) + (on ? "=1\n" : "=0\n");
    }
    store.set_value(settings_key(path, "active"), stored);
}
