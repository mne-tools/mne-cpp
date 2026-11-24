//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raw_command.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Untyped intermediate produced by @ref RTSERVER::CommandParser before type information is bound to parameters.
 *
 * @ref RTSERVER::RawCommand is the first object the parser builds when a
 * command arrives on the command port of mne_rt_server. At that point the wire payload
 * has been tokenised into a command keyword and a list of string
 * arguments, but the parser does not yet know which @ref RTSERVER::Command
 * descriptor (if any) those arguments belong to — the parameter types,
 * names and descriptions live in whichever @ref RTSERVER::CommandManager ends up
 * accepting the request. Keeping that pre-resolution state in its own
 * class avoids forcing every observer to deal with half-populated
 * @c Command objects.
 *
 * The class also implements @ref RTSERVER::ICommand so it can be
 * notified through the same dispatch path as a fully-typed @c Command;
 * the callbacks registered with @c connectExecuted hand the raw parameter list
 * to whichever @ref RTSERVER::CommandManager recognises the keyword, which is
 * responsible for converting the strings into the appropriate
 * @ref RTSERVER::CommandValue values and executing them as a typed @c Command.
 *
 * @c isJson() records whether the original payload was JSON or CLI, so
 * the reply produced by the eventual handler can be sent back in the
 * same dialect the caller used.
 */

#ifndef RAWCOMMAND_H
#define RAWCOMMAND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_connector_global.h"
#include "command_pattern.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
/**
 * @brief Tokenised but unresolved command: a keyword plus a list of raw string arguments awaiting type binding.
 *
 * Built by @ref CommandParser before any @ref CommandManager has
 * claimed the request. Observers turn the string arguments
 * into typed @ref Command parameters once the matching schema is
 * located. Retains the JSON-vs-CLI dialect flag so replies travel back
 * in the same encoding the caller used.
 */
class RTSERVER_CONNECTOR_EXPORT RawCommand : public ICommand
{
public:
    /** Called with the raw parameters when the command is executed. */
    using ExecutedCallback = std::function<void(const std::vector<std::string>&)>;

    //=========================================================================================================
    /**
     * Default constructor.
     */
    RawCommand();

    //=========================================================================================================
    /**
     * Constructs a raw command without parameters.
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     */
    explicit RawCommand(const std::string& p_sCommand, bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Copy constructor. Copies the data, not the registered callbacks.
     *
     * @param[in] p_rawCommand   RawCommand which should be copied.
     */
    RawCommand(const RawCommand& p_rawCommand);

    //=========================================================================================================
    /**
     * @return The command keyword.
     */
    inline const std::string& command() const;

    //=========================================================================================================
    /**
     * @return The number of parameters.
     */
    inline std::uint32_t count() const;

    //=========================================================================================================
    /**
     * Runs the callbacks registered with @ref connectExecuted with the raw parameters.
     */
    void execute() override;

    //=========================================================================================================
    /**
     * Registers a callback of @ref execute.
     *
     * @param[in] callback   Called with the raw parameters.
     */
    void connectExecuted(ExecutedCallback callback);

    //=========================================================================================================
    /**
     * @return Whether the received command was in JSON format.
     */
    inline bool isJson() const;

    //=========================================================================================================
    /**
     * @return The raw parameter values.
     */
    inline std::vector<std::string>& pValues();

    //=========================================================================================================
    /**
     * @return The raw parameter values.
     */
    inline const std::vector<std::string>& pValues() const;

    //=========================================================================================================
    /**
     * Assignment operator; keeps the registered callbacks of this command.
     *
     * @param[in] rhs    RawCommand which should be assigned.
     *
     * @return Reference to this object.
     */
    RawCommand& operator=(const RawCommand& rhs);

private:
    std::string m_sCommand;                        /**< The command keyword. */
    bool m_bIsJson;                                /**< Whether the command was received as JSON. */
    std::vector<std::string> m_qListRawParameters; /**< Raw parameters. Their type is not specified yet. */
    std::vector<ExecutedCallback> m_executed;      /**< Callbacks of @ref execute. */
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

inline const std::string& RawCommand::command() const
{
    return m_sCommand;
}

//=============================================================================================================

inline std::uint32_t RawCommand::count() const
{
    return static_cast<std::uint32_t>(m_qListRawParameters.size());
}

//=============================================================================================================

inline bool RawCommand::isJson() const
{
    return m_bIsJson;
}

//=============================================================================================================

inline std::vector<std::string>& RawCommand::pValues()
{
    return m_qListRawParameters;
}

//=============================================================================================================

inline const std::vector<std::string>& RawCommand::pValues() const
{
    return m_qListRawParameters;
}

} // NAMESPACE

#endif // RAWCOMMAND_H
