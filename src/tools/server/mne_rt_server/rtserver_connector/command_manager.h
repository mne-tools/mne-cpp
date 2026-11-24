//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_manager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-component registry of @ref RTSERVER::Command schemas; receives @ref RTSERVER::RawCommand from the parser and dispatches matching @c Command instances.
 *
 * @ref RTSERVER::CommandManager is the receiver half of the observer
 * pattern anchored at @ref RTSERVER::CommandParser. Each subsystem that
 * understands a slice of the @c mne_rt_server vocabulary (acquisition
 * control, source-estimation control, GUI plumbing, etc.) owns a
 * @c CommandManager seeded with the @c Command schemas it accepts and
 * attaches it to the shared @c CommandParser. When the parser fans an
 * incoming @c RawCommand out, every manager checks its own map and
 * silently ignores requests it does not own — there is no global
 * dispatch table to keep in sync.
 *
 * Schemas can be installed three different ways: explicitly by name and
 * description via @c insert(std::string, std::string), one at a time by handing
 * over a fully-formed @c Command via @c insert(std::string, Command) (the
 * manager becomes the owner that routes the replies of the stored copy),
 * or in bulk by loading a @c mne::JsonDocument that describes a whole
 * command set in the same JSON layout the server itself emits when a
 * client asks for its command list. The CTOR overload that takes a
 * JSON text or a @c mne::JsonDocument is the convenience entry point for
 * that bulk path.
 *
 * Outbound traffic uses three callback channels. @c commandMapChanged() notifies
 * the rest of the application when the schema set has been mutated.
 * @c triggered(Command) lets the manager push outbound requests onto
 * the command port. @c response(std::string, Command) carries the
 * reply produced by an executed @c Command back to whoever originally
 * sent it. @c m_bIsActive lets a subsystem temporarily mute the manager
 * (during long-running operations, for example) without detaching it
 * from the parser.
 */

#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_connector_global.h"
#include "command.h"
#include "command_parser.h"
#include "observer_pattern.h"

#include <mne/base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
/**
 * @brief Registry of the commands one component understands; executes the parsed commands that match.
 */
class RTSERVER_CONNECTOR_EXPORT CommandManager : public IObserver
{
public:
    using CommandMapChangedCallback = std::function<void()>;                                /**< Schema set changed. */
    using TriggeredCallback = std::function<void(const Command&)>;                          /**< Command sent. */
    using ResponseCallback = std::function<void(const std::string& reply, const Command&)>; /**< Reply of a command. */

    //=========================================================================================================
    /**
     * Constructs an empty manager.
     *
     * @param[in] p_bIsActive    If the manager executes parsed commands (optional, default true).
     */
    explicit CommandManager(bool p_bIsActive = true);

    //=========================================================================================================
    /**
     * Constructs a manager with the commands of a JSON command description.
     *
     * @param[in] p_sJsonDoc     JSON text holding a "commands" object.
     * @param[in] p_bIsActive    If the manager executes parsed commands (optional, default true).
     */
    explicit CommandManager(std::string_view p_sJsonDoc, bool p_bIsActive = true);

    //=========================================================================================================
    /**
     * Constructs a manager with the commands of a JSON command description.
     *
     * @param[in] p_jsonDoc      Document holding a "commands" object.
     * @param[in] p_bIsActive    If the manager executes parsed commands (optional, default true).
     */
    explicit CommandManager(const mne::JsonDocument& p_jsonDoc, bool p_bIsActive = true);

    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    ~CommandManager() override;

    //=========================================================================================================
    /**
     * Removes all commands.
     */
    void clear();

    //=========================================================================================================
    /**
     * @return The commands by keyword.
     */
    inline std::map<std::string, Command>& commandMap();

    //=========================================================================================================
    /**
     * @param[in] p_sCommand     Command keyword.
     *
     * @return Whether the manager knows the command.
     */
    inline bool hasCommand(const std::string& p_sCommand) const;

    //=========================================================================================================
    /**
     * Inserts the commands of a JSON command description; existing commands are kept.
     *
     * @param[in] p_jsonDocument     Document holding a "commands" object.
     */
    void insert(const mne::JsonDocument& p_jsonDocument);

    //=========================================================================================================
    /**
     * Inserts or replaces a command without parameters.
     *
     * @param[in] p_sKey             Command keyword.
     * @param[in] p_sDescription     Command description.
     */
    void insert(const std::string& p_sKey, const std::string& p_sDescription);

    //=========================================================================================================
    /**
     * Inserts or replaces a command.
     *
     * @param[in] p_sKey     Command keyword.
     * @param[in] p_command  The command.
     */
    void insert(const std::string& p_sKey, const Command& p_command);

    //=========================================================================================================
    /**
     * @return Whether the manager executes parsed commands.
     */
    inline bool isActive() const;

    //=========================================================================================================
    /**
     * @param[in] status     Whether the manager executes parsed commands.
     */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
     * Executes the command the parser has just parsed, if this manager knows it and its parameters convert.
     *
     * @param[in] p_pSubject     The notifying @ref CommandParser.
     */
    void update(Subject* p_pSubject) override;

    void connectCommandMapChanged(CommandMapChangedCallback callback); /**< Registers a schema-change callback. */
    void connectTriggered(TriggeredCallback callback);                 /**< Registers a callback of @ref triggered. */
    void connectResponse(ResponseCallback callback);                   /**< Registers a callback of @ref response. */

    //=========================================================================================================
    /**
     * Runs the triggered callbacks.
     *
     * @param[in] p_command  The command to send.
     */
    void triggered(const Command& p_command);

    //=========================================================================================================
    /**
     * Runs the response callbacks.
     *
     * @param[in] p_sReply   Reply text.
     * @param[in] p_command  The command replied to.
     */
    void response(const std::string& p_sReply, const Command& p_command);

    //=========================================================================================================
    /**
     * @param[in] key    Command keyword.
     *
     * @return The command; a default command is inserted if there is none.
     */
    Command& operator[](const std::string& key);

    //=========================================================================================================
    /**
     * @param[in] key    Command keyword.
     *
     * @return A copy of the command, or a default command if there is none.
     */
    Command operator[](const std::string& key) const;

private:
    void commandMapChanged();

    bool m_bIsActive;                                           /**< Whether parsed commands are executed. */
    mne::JsonDocument m_jsonDocumentOrigin;                     /**< Document the manager was constructed from. */
    std::map<std::string, Command> m_qMapCommands;              /**< Lookup table of the available commands. */
    std::vector<CommandMapChangedCallback> m_commandMapChanged; /**< Callbacks of a schema change. */
    std::vector<TriggeredCallback> m_triggered;                 /**< Callbacks of @ref triggered. */
    std::vector<ResponseCallback> m_response;                   /**< Callbacks of @ref response. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::map<std::string, Command>& CommandManager::commandMap()
{
    return m_qMapCommands;
}

//=============================================================================================================

inline bool CommandManager::hasCommand(const std::string& p_sCommand) const
{
    return m_qMapCommands.contains(p_sCommand);
}

//=============================================================================================================

inline bool CommandManager::isActive() const
{
    return m_bIsActive;
}

//=============================================================================================================

inline void CommandManager::setStatus(bool status)
{
    m_bIsActive = status;
}

} // NAMESPACE

#endif // COMMUNICATIONMANAGER_H
