//=============================================================================================================
/**
 * @file     rawcommand.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     Definition of the RawCommand Class.
 *
 */

//=============================================================================================================
// Includes
//=============================================================================================================

#include "rawcommand.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;

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
