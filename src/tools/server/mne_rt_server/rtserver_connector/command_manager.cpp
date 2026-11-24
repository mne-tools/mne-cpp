//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_manager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Translation unit for @ref RTSERVER::CommandManager: schema storage, observer-side dispatch and JSON bulk-load.
 *
 * Implements the callback channels (@c triggered / @c response /
 * schema changes), the
 * three @c insert overloads (single keyword, full @ref Command,
 * @c mne::JsonDocument bulk import), and the @c IObserver::update() hook
 * called by @ref CommandParser when a new @ref RawCommand has arrived.
 * The @c m_qMapCommands lookup table backs both the @c [] operator and
 * @c hasCommand() so attached clients can introspect the accepted
 * vocabulary without re-querying the server.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "command_manager.h"
#include "raw_command.h"

#include <mne/base/log.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandManager::CommandManager(bool p_bIsActive)
: m_bIsActive(p_bIsActive)
{
}

//=============================================================================================================

CommandManager::CommandManager(std::string_view p_sJsonDoc, bool p_bIsActive)
: m_bIsActive(p_bIsActive)
, m_jsonDocumentOrigin(mne::JsonDocument::from_json(p_sJsonDoc))
{
    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::CommandManager(const mne::JsonDocument& p_jsonDoc, bool p_bIsActive)
: m_bIsActive(p_bIsActive)
, m_jsonDocumentOrigin(p_jsonDoc)
{
    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::~CommandManager() = default;

//=============================================================================================================

void CommandManager::clear()
{
    m_qMapCommands.clear();
}

//=============================================================================================================

void CommandManager::insert(const mne::JsonDocument& p_jsonDocument)
{
    if (!p_jsonDocument.is_object() || p_jsonDocument.object().value("commands").is_undefined()) {
        return;
    }

    for (const auto& [key, content] : p_jsonDocument.object().value("commands").to_object()) {
        if (!m_qMapCommands.contains(key)) {
            Command t_command(key, content.to_object(), true);
            t_command.setManager(this);
            m_qMapCommands.emplace(key, t_command);
        } else {
            mne::Log::warning("Warning: CommandMap contains command {} already. Insertion skipped.\n", key);
        }
    }

    commandMapChanged();
}

//=============================================================================================================

void CommandManager::insert(const std::string& p_sKey, const std::string& p_sDescription)
{
    Command t_command(p_sKey, p_sDescription, false);
    insert(p_sKey, t_command);
}

//=============================================================================================================

void CommandManager::insert(const std::string& p_sKey, const Command& p_command)
{
    Command t_command(p_command);
    t_command.setManager(this);
    m_qMapCommands.insert_or_assign(p_sKey, t_command);
    commandMapChanged();
}

//=============================================================================================================

void CommandManager::update(Subject* p_pSubject)
{
    // If Manager is not active do not parse commands
    if (!m_bIsActive) {
        return;
    }

    auto* t_pCommandParser = static_cast<CommandParser*>(p_pSubject);

    const RawCommand t_rawCommand(t_pCommandParser->getRawCommand());
    const auto it = m_qMapCommands.find(t_rawCommand.command());
    if (it == m_qMapCommands.end()) {
        return;
    }
    Command& t_command = it->second;

    // check if number of parameters is right
    if (t_rawCommand.count() >= t_command.count()) {
        t_command.isJson() = t_rawCommand.isJson();

        //Parse Parameters
        for (std::uint32_t i = 0; i < t_command.count(); ++i) {
            if (!t_command.pValues()[i].convertFrom(t_rawCommand.pValues()[i])) {
                return;
            }
        }

        t_command.execute();
    }
}

//=============================================================================================================

void CommandManager::connectCommandMapChanged(CommandMapChangedCallback callback)
{
    m_commandMapChanged.push_back(std::move(callback));
}

//=============================================================================================================

void CommandManager::connectTriggered(TriggeredCallback callback)
{
    m_triggered.push_back(std::move(callback));
}

//=============================================================================================================

void CommandManager::connectResponse(ResponseCallback callback)
{
    m_response.push_back(std::move(callback));
}

//=============================================================================================================

void CommandManager::triggered(const Command& p_command)
{
    for (const TriggeredCallback& callback : m_triggered) {
        callback(p_command);
    }
}

//=============================================================================================================

void CommandManager::response(const std::string& p_sReply, const Command& p_command)
{
    for (const ResponseCallback& callback : m_response) {
        callback(p_sReply, p_command);
    }
}

//=============================================================================================================

void CommandManager::commandMapChanged()
{
    for (const CommandMapChangedCallback& callback : m_commandMapChanged) {
        callback();
    }
}

//=============================================================================================================

Command& CommandManager::operator[](const std::string& key)
{
    return m_qMapCommands[key];
}

//=============================================================================================================

Command CommandManager::operator[](const std::string& key) const
{
    const auto it = m_qMapCommands.find(key);
    return it != m_qMapCommands.end() ? it->second : Command();
}
