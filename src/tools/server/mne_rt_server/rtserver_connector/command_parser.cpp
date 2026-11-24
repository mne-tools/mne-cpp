//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_parser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Translation unit for @ref RTSERVER::CommandParser: JSON/CLI tokenisation and fan-out to attached managers.
 *
 * Implements the dual-dialect @c parse() entry point. CLI input is
 * split on whitespace into a keyword and string arguments; JSON input
 * is decoded with @c mne::JsonDocument and may contain either a single
 * command object or an array of them, supporting batched requests in a
 * single payload. In either case the result is a populated
 * @ref RawCommand handed to every attached @ref CommandManager via the
 * @c Subject::notify mechanism inherited from @ref RTSERVER::Subject; the
 * @c response callbacks carry replies produced by handlers back to the
 * originating client of the command port.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "command_parser.h"
#include "command_manager.h"

#include <mne/base/json.h>
#include <mne/base/log.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool CommandParser::exists(const std::string& p_sCommand)
{
    for (IObserver* observer : observers()) {
        auto* t_pCommandManager = static_cast<CommandManager*>(observer);
        if (t_pCommandManager->hasCommand(p_sCommand)) {
            return true;
        }
    }
    return false;
}

//=============================================================================================================

bool CommandParser::parse(const std::string& p_sInput, mne::StringList& p_qListCommandsParsed)
{
    if (p_sInput.empty()) {
        return false;
    }

    p_qListCommandsParsed.clear();

    //Check if JSON format;
    if (p_sInput.front() == '{') {
        mne::Log::debug("JSON command recognized");

        const mne::JsonDocument t_jsonDocument = mne::JsonDocument::from_json(p_sInput);

        //Switch to command object
        if (!t_jsonDocument.is_object() || t_jsonDocument.object().value("commands").is_undefined()) {
            return false;
        }

        //iterate over commands
        for (const auto& [name, parameters] : t_jsonDocument.object().value("commands").to_object()) {
            //Print Command
            mne::Log::info("{}\r\n", name);

            if (exists(name)) {
                m_rawCommand = RawCommand(name, true);

                // push command to processed commands
                p_qListCommandsParsed.push_back(name);

                //append the parameters
                for (const auto& [key, value] : parameters.to_object()) {
                    mne::Log::info(" {}", value.to_string());
                    //ToDo do a cross check with the param naming and key
                    m_rawCommand.pValues().emplace_back(value.to_string());
                }

                //Notify attached command manager
                notify();
            }
            mne::Log::info("\r\n");
        }
    } else {
        const mne::StringList t_qCommandList = mne::split(p_sInput, ' ');

        //Print command
        mne::Log::info("{}\r\n", t_qCommandList[0]);

        if (!exists(t_qCommandList[0])) {
            mne::Log::info("\r\n");
            return false;
        }

        m_rawCommand = RawCommand(t_qCommandList[0], false);

        // push command to processed commands
        p_qListCommandsParsed.push_back(t_qCommandList[0]);

        //Parse Parameters
        for (std::size_t i = 1; i < t_qCommandList.size(); ++i) {
            mne::Log::info(" {}", t_qCommandList[i]);
            m_rawCommand.pValues().push_back(t_qCommandList[i]);
        }
        mne::Log::info("\r\n");
        notify();
    }

    return true;
}

//=============================================================================================================

void CommandParser::connectResponse(ResponseCallback callback)
{
    m_response.push_back(std::move(callback));
}

//=============================================================================================================

void CommandParser::response(const std::string& p_sResponse, const Command& p_command)
{
    for (const ResponseCallback& callback : m_response) {
        callback(p_sResponse, p_command);
    }
}
