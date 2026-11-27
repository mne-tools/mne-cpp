//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Panel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "panel.h"
#include "../host/settings_store.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Panel::Panel(std::string id, std::string title)
: m_id(std::move(id))
, m_title(title)
, m_form(std::make_shared<SettingsForm>(std::move(title)))
{
}

//=============================================================================================================

Panel::~Panel() = default;

//=============================================================================================================

void Panel::set_gui_mode(GuiMode mode)
{
    if (mode != m_guiMode) {
        m_guiMode = mode;
        gui_mode_changed();
    }
}

//=============================================================================================================

void Panel::set_processing_mode(ProcessingMode mode)
{
    if (mode != m_processingMode) {
        m_processingMode = mode;
        processing_mode_changed();
    }
}

//=============================================================================================================

void Panel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    // Listeners may change the fields while values are restored
    const std::vector<SettingField> fields = m_form->fields();
    for (const SettingField& field : fields) {
        if (!field.persistent) {
            continue;
        }
        // Stores may hand back numbers and flags as text, so they are read as the field's type
        const std::string key = settings_key(path, field.key);
        if (!store.contains(key)) {
            continue;
        }
        switch (field.kind) {
            case SettingField::Kind::Bool:
                m_form->set_value(field.key, store.read_bool(key, m_form->get<bool>(field.key)));
                break;
            case SettingField::Kind::Int:
                m_form->set_value(field.key, store.read_int(key, m_form->get<int>(field.key)));
                break;
            case SettingField::Kind::Double:
                m_form->set_value(field.key, store.read_double(key, m_form->get<double>(field.key)));
                break;
            default:
                m_form->set_value(field.key, store.read_string(key, m_form->get<std::string>(field.key)));
                break;
        }
    }
}

//=============================================================================================================

void Panel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    for (const auto& [key, value] : m_form->values()) {
        if (const SettingField* field = m_form->field(key); field && field->persistent) {
            store.set_value(settings_key(path, key), value);
        }
    }
}

//=============================================================================================================

std::string Panel::settings_key(const std::string& path, const std::string& name) const
{
    std::string key = path;
    if (!key.empty() && key.back() != '/') {
        key += '/';
    }
    return key + m_id + '/' + name;
}

//=============================================================================================================

void Panel::gui_mode_changed()
{
}

//=============================================================================================================

void Panel::processing_mode_changed()
{
}
