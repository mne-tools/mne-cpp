//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_parser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Front-end tokeniser that turns incoming JSON or CLI payloads into @ref RTSERVER::RawCommand objects and notifies attached managers.
 *
 * @ref RTSERVER::CommandParser is the @ref RTSERVER::Subject side of the observer
 * pattern; every @ref RTSERVER::CommandManager that
 * wants to react to a slice of the command vocabulary attaches itself,
 * and the parser then fans every accepted request out to all of them.
 * That decoupling lets multiple subsystems (data acquisition, source
 * estimation, GUI plumbing) each register only the commands they know
 * how to handle without growing a single monolithic dispatcher.
 *
 * Two wire dialects are accepted on the same entry point. CLI input is
 * whitespace-tokenised into a command keyword plus string arguments;
 * JSON input is decoded with @c mne::JsonDocument and may contain either a
 * single command object or an array of commands, allowing batched
 * requests in one round-trip. In either case the result is the same
 * untyped @ref RTSERVER::RawCommand handed to the observers — the type-aware
 * @ref RTSERVER::Command is constructed downstream once a manager has identified
 * which schema applies.
 *
 * @c parse() returns @c true when at least one observer claimed the
 * input and additionally records the names of the recognised commands
 * in @c p_qListCommandsParsed; the @c response callbacks (see
 * @c connectResponse) are the reverse channel used by managers to push reply text back to
 * whichever socket originally produced the request.
 */

#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_connector_global.h"
#include "raw_command.h"
#include "command.h"
#include "observer_pattern.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
 * @brief Parses JSON or CLI command input into a @ref RawCommand and notifies the attached managers.
 */
class RTSERVER_CONNECTOR_EXPORT CommandParser : public Subject
{
public:
    /** Called with a reply and the command it answers. */
    using ResponseCallback = std::function<void(const std::string& reply, const Command&)>;

    CommandParser() = default;

    //=========================================================================================================
    /**
     * @param[in] p_sCommand     Command keyword.
     *
     * @return Whether one of the attached managers knows the command.
     */
    bool exists(const std::string& p_sCommand);

    //=========================================================================================================
    /**
     * Parses the input and notifies the attached managers of every known command.
     *
     * @param[in] p_sInput               JSON ("{...}") or CLI input.
     * @param[out] p_qListCommandsParsed The keywords of the known commands of the input.
     *
     * @return False if the input is empty, invalid or (CLI) names an unknown command.
     */
    bool parse(const std::string& p_sInput, mne::StringList& p_qListCommandsParsed);

    //=========================================================================================================
    /**
     * @return The raw command parsed last.
     */
    inline RawCommand& getRawCommand();

    //=========================================================================================================
    /**
     * Registers a callback of @ref response.
     *
     * @param[in] callback   Called with every reply.
     */
    void connectResponse(ResponseCallback callback);

    //=========================================================================================================
    /**
     * Passes a reply to the registered response callbacks.
     *
     * @param[in] p_sResponse    Reply text.
     * @param[in] p_command      The command replied to.
     */
    void response(const std::string& p_sResponse, const Command& p_command);

private:
    RawCommand m_rawCommand;                  /**< The raw command parsed last. */
    std::vector<ResponseCallback> m_response; /**< Callbacks of @ref response. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

RawCommand& CommandParser::getRawCommand()
{
    return m_rawCommand;
}

} // NAMESPACE

#endif // COMMANDPARSER_H
