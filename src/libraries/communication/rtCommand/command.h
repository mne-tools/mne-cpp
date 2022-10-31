//=============================================================================================================
/**
 * @file     command.h
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
 * @brief    Declaration of the Command Class.
 *
 */

#ifndef COMMAND_H
#define COMMAND_H

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
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QSharedPointer>
#include <QDebug>
#include <QPair>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

static QVariant defaultVariant;

//=============================================================================================================
/**
 * Command, which includes beside command name also command parameters
 *
 * @brief Command
 */
class COMMUNICATIONSHARED_EXPORT Command: public QObject, public UTILSLIB::ICommand
{
Q_OBJECT

public:
    typedef QSharedPointer<Command> SPtr;
    typedef QSharedPointer<const Command> ConstSPtr;

    //=========================================================================================================
    /**
     * Constructs a Command
     *
     * @param[in] p_bIsJson      If is received/should be send as JSON (optional, default true).
     * @param[in] parent         Parent QObject (optional).
     */
    explicit Command(bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which parses a command stored in a json object
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_qCommandContent  Content encapsulated in a JsonObject.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QJsonObject &p_qCommandContent, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructs a command without parameters
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_sDescription     Command description.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which assembles a command from single parts
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_sDescription     Command description.
     * @param[in] p_qListParamNames  Parameter names.
     * @param[in] p_qListParamValues Parameter values/types.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which assembles a command from single parts
     *
     * @param[in] p_sCommand                 Command.
     * @param[in] p_sDescription             Command description.
     * @param[in] p_qListParamNames          Parameter names.
     * @param[in] p_qListParamValues         Parameter values/types.
     * @param[in] p_vecParameterDescriptions Parameter descriptions;.
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, const QStringList &p_vecParameterDescriptions, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_Command      Command to be copied.
     */
    Command(const Command &p_Command);

    //=========================================================================================================
    /**
     * Destroys the command
     */
    virtual ~Command();

    //=========================================================================================================
    /**
     * Short command for this request
     *
     * @return Short command representation.
     */
    inline QString command() const;

    //=========================================================================================================
    /**
     * Returns the number of parameters.
     *
     * @return number of parameters.
     */
    inline quint32 count() const;

    //=========================================================================================================
    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    inline QString description() const;

    //=========================================================================================================
    /**
     * Inherited by ICommand
     *
     * @return  emits received.
     */
    virtual void execute();

    //=========================================================================================================
    /**
     * If received command was Json fomratted or triggered command should be Json formatted.
     *
     * @return Json formatted.
     */
    inline bool& isJson();

    //=========================================================================================================
    /**
     * Get parameter descriptions
     *
     * @return parameter descriptions.
     */
    inline QList<QString> pDescriptions() const;

    //=========================================================================================================
    /**
     * Get parameter names
     *
     * @return parameter names.
     */
    inline QList<QString> pNames() const;

    //=========================================================================================================
    /**
     * Returns parameter values
     *
     * @return parameter values.
     */
    inline QList<QVariant>& pValues();

    //=========================================================================================================
    /**
     * Inherited command reply channel.
     *
     * @param[in] p_sReply   command reply.
     */
    void reply(const QString &p_sReply);

    //=========================================================================================================
    /**
     * Sender slot which emmits triggered signal
     */
    void send();

    //=========================================================================================================
    /**
     * Creates a JSON Command Object
     *
     * @return Command converted to a JSON Object.
     */
    QJsonObject toJsonObject() const;

    //=========================================================================================================
    /**
     * Creates a StringList with three items. First item is the command, second the parameter list and thrid the description.
     *
     * @return Command as a StringList.
     */
    QStringList toStringList() const;

    //=========================================================================================================
    /**
     * Creates a string JSON formatted ready send command.
     *
     * @return Command as a JSON formatted string which contains parameter values too.
     */
    QString toStringReadySend() const;

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     Command which should be assigned.
     */
    Command& operator= (const Command &rhs);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by name
     *
     * @param[in] key    the parameter name.
     *
     * @return Parameter value related to the parameter name.
     */
    QVariant& operator[] (const QString &key);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idx    the parameter index.
     *
     * @return Parameter value related to the parameter index.
     */
    QVariant& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator []
     *
     * @param[in] key    the parameter name.
     *
     * @return Parameter value related to the parameter name.
     */
    const QVariant operator[] (const QString &key) const;

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when command patterns execute method is processed.
     *
     * @param[in] p_command  the executed command.
     */
    void executed(Command p_command);

public:
    QString             m_sCommand;
    QString             m_sDescription;
    QStringList         m_qListParamNames;
    QList<QVariant>     m_qListParamValues;
    QStringList         m_qListParamDescriptions;
    bool                m_bIsJson;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QString Command::command() const
{
    return m_sCommand;
}

//=============================================================================================================

inline quint32 Command::count() const
{
    return m_qListParamValues.size();
}

//=============================================================================================================

inline QString Command::description() const
{
    return m_sDescription;
}

//=============================================================================================================

inline bool& Command::isJson()
{
    return m_bIsJson;
}

//=============================================================================================================

inline QList<QString> Command::pDescriptions() const
{
    return m_qListParamDescriptions;
}

//=============================================================================================================

inline QList<QString> Command::pNames() const
{
    return m_qListParamNames;
}

//=============================================================================================================

inline QList<QVariant>& Command::pValues()
{
    return m_qListParamValues;
}
} // NAMESPACE

#endif // COMMAND_H
