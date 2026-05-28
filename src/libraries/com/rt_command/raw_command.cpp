//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file raw_command.cpp
 * @since 2026
 * @date  March 2026
 * @brief Translation unit for @ref COMLIB::RawCommand: storage of the untyped keyword/argument pair before schema binding.
 *
 * Implements the trivial constructors, copy-assignment and the
 * @c ICommand::execute() override that emits @c executed(QList<QString>)
 * so a @ref CommandManager can pick the raw arguments up, look the
 * keyword up in its schema map, and synthesise a typed @ref Command.
 * The JSON-vs-CLI flag carried alongside the arguments lets the eventual
 * reply traverse the same dialect the request arrived in.
 */

//=============================================================================================================
// Includes
//=============================================================================================================

#include "raw_command.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawCommand::RawCommand(QObject *parent)
: QObject(parent)
, m_bIsJson(false)
{
}

//=============================================================================================================

RawCommand::RawCommand(const QString &p_sCommand, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

RawCommand::RawCommand(const RawCommand &p_rawCommand)
: QObject(p_rawCommand.parent())
, m_sCommand(p_rawCommand.m_sCommand)
, m_bIsJson(p_rawCommand.m_bIsJson)
, m_qListRawParameters(p_rawCommand.m_qListRawParameters)
{
}

//=============================================================================================================

void RawCommand::execute()
{
    emit executed(m_qListRawParameters);
}

//=============================================================================================================

RawCommand& RawCommand::operator= (const RawCommand &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        m_sCommand = rhs.m_sCommand;
        m_bIsJson = rhs.m_bIsJson;
        m_qListRawParameters = rhs.m_qListRawParameters;
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}
