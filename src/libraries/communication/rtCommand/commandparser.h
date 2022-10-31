//=============================================================================================================
/**
 * @file     commandparser.h
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
 * @brief    Declaration of the CommandParser Class.
 *
 */

#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../communication_global.h"
#include "rawcommand.h"
#include "command.h"

#include <utils/generics/observerpattern.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>
#include <QMultiMap>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

class COMMUNICATIONSHARED_EXPORT CommandParser : public QObject, public UTILSLIB::Subject
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Default constructor
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit CommandParser(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Checks if a command exists
     *
     * @param[in] p_sCommand     Command to check.
     *
     * @return true if command exists, false otherwise.
     */
    bool exists(const QString& p_sCommand);

    //=========================================================================================================
    /**
     * Parses a CLI command or JSON command (list) and notifies all attached observers (command managers)
     *
     * @param[in] p_sInput               Input to parse.
     * @param[out] p_qListCommandsParsed  List of parsed commands.
     */
    bool parse(const QString &p_sInput, QStringList &p_qListCommandsParsed);

    //=========================================================================================================
    /**
     * Returns the stored RawCommand
     *
     * @return the stored RawCommand.
     */
    inline RawCommand& getRawCommand();

signals:
    //=========================================================================================================
    /**
     * Response channel which is used by attached observers (command managers) to send data back to subject
     *
     *@param[in] p_sResponse     Observer response/data.
     *@param[in] p_command       Command which send the response
     */
    void response(QString p_sResponse, Command p_command);

private:
    RawCommand m_rawCommand;
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
