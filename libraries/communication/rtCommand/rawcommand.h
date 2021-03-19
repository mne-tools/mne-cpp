//=============================================================================================================
/**
 * @file     rawcommand.h
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
 * @brief    Declaration of the RawCommand Class.
 *
 */

#ifndef RAWCOMMAND_H
#define RAWCOMMAND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../communication_global.h"

#include <utils/generics/commandpattern.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QVariant>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

//=============================================================================================================
/**
 * RawCommand, which includes beside command name also command parameters. The parameter type is not jet specified.
 *
 * @brief RawCommand
 */
class COMMUNICATIONSHARED_EXPORT RawCommand : public QObject, public UTILSLIB::ICommand
{
Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] parent             Parent QObject (optional).
     */
    explicit RawCommand(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which parses a command stored in a json object
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit RawCommand(const QString &p_sCommand, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_rawCommand   RawCommand which should be copied.
     */
    explicit RawCommand(const RawCommand &p_rawCommand);

    //=========================================================================================================
    /**
     * Command name
     *
     * @return short command representation.
     */
    inline QString command() const;

    //=========================================================================================================
    /**
     * Returns the number of parameters.
     *
     * @return number of parameters.
     */
    inline quint32 count() const;

    virtual void execute();

    //=========================================================================================================
    /**
     * Returns whether the received command was in Json format.
     *
     * @return true if received command was in Json format, false otherwise.
     */
    inline bool isJson() const;

    //=========================================================================================================
    /**
     * Returns parameter values
     *
     * @return parameter values.
     */
    inline QList<QString>& pValues();

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs    RawCommand which should be assigned.
     */
    RawCommand& operator= (const RawCommand &rhs);

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when command patterns execute method is processed.
     *
     * @param[in] p_qListParameters    Parameter List.
     */
    void executed(QList<QString> p_qListParameters);

private:
    QString m_sCommand;
    bool m_bIsJson;

    QList<QString> m_qListRawParameters;    /**< Raw parameters. Their type is not specified jet.*/
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString RawCommand::command() const
{
    return m_sCommand;
}

//=============================================================================================================

quint32 RawCommand::count() const
{
    return m_qListRawParameters.size();
}

//=============================================================================================================

bool RawCommand::isJson() const
{
    return m_bIsJson;
}

//=============================================================================================================

QList<QString>& RawCommand::pValues()
{
    return m_qListRawParameters;
}
} // NAMESPACE

#endif // RAWCOMMAND_H
