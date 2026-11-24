//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raw_command.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Translation unit for @ref RTSERVER::RawCommand: storage of the untyped keyword/argument pair before schema binding.
 *
 * Implements the trivial constructors, copy-assignment and the
 * @c ICommand::execute() override that passes the raw parameters to the registered callbacks
 * so a @ref CommandManager can pick the raw arguments up, look the
 * keyword up in its schema map, and synthesise a typed @ref Command.
 * The JSON-vs-CLI flag carried alongside the arguments lets the eventual
 * reply traverse the same dialect the request arrived in.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "raw_command.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawCommand::RawCommand()
: m_bIsJson(false)
{
}

//=============================================================================================================

RawCommand::RawCommand(const std::string& p_sCommand, bool p_bIsJson)
: m_sCommand(p_sCommand)
, m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

RawCommand::RawCommand(const RawCommand& p_rawCommand)
: ICommand(p_rawCommand)
, m_sCommand(p_rawCommand.m_sCommand)
, m_bIsJson(p_rawCommand.m_bIsJson)
, m_qListRawParameters(p_rawCommand.m_qListRawParameters)
{
}

//=============================================================================================================

void RawCommand::execute()
{
    for (const ExecutedCallback& callback : m_executed) {
        callback(m_qListRawParameters);
    }
}

//=============================================================================================================

void RawCommand::connectExecuted(ExecutedCallback callback)
{
    m_executed.push_back(std::move(callback));
}

//=============================================================================================================

RawCommand& RawCommand::operator=(const RawCommand& rhs)
{
    if (this != &rhs) {
        m_sCommand = rhs.m_sCommand;
        m_bIsJson = rhs.m_bIsJson;
        m_qListRawParameters = rhs.m_qListRawParameters;
    }
    return *this;
}
