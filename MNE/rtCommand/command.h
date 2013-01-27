//=============================================================================================================
/**
* @file     command.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcommand_global.h"

#include <generics/commandpattern.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTCOMMANDLIB
//=============================================================================================================

namespace RTCOMMANDLIB
{

static QVariant defaultVariant;


//=============================================================================================================
/**
* Command, which includes beside command name also command parameters
*
* @brief Command
*/
class RTCOMMANDSHARED_EXPORT Command: public QObject, public ICommand
{
Q_OBJECT
public:
    typedef QSharedPointer<Command> SPtr;
    typedef QSharedPointer<const Command> ConstSPtr;

    //=========================================================================================================
    /**
    * constructor.
    *
    * @param[in] p_bIsJson      If is received/should be send as JSON (optional, default true)
    * @param[in] parent         Parent QObject (optional)
    */
    Command(bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructor which parses a command stored in a json object
    *
    * @param[in] p_sCommand         Command
    * @param[in] p_qCommandContent  Content encapsulated in a JsonObject
    * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true)
    * @param[in] parent             Parent QObject (optional)
    */
    explicit Command(const QString &p_sCommand, const QJsonObject &p_qCommandContent, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructs a command without parameters
    *
    * @param[in] p_sCommand         Command
    * @param[in] p_sDescription     Command description
    * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true)
    * @param[in] parent             Parent QObject (optional)
    */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructor which assembles a command from single parts
    *
    * @param[in] p_sCommand         Command
    * @param[in] p_sDescription     Command description
    * @param[in] p_mapParameters    Parameter names + values/types.
    * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true)
    * @param[in] parent             Parent QObject (optional)
    */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QMap<QString, QVariant> &p_mapParameters, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructor which assembles a command from single parts
    *
    * @param[in] p_sCommand                 Command
    * @param[in] p_sDescription             Command description
    * @param[in] p_mapParameters            Parameter names + values/types.
    * @param[in] p_vecParameterDescriptions Parameter descriptions;
    */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QMap<QString, QVariant> &p_mapParameters, const QList<QString> &p_vecParameterDescriptions, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_Command      Command to be copied
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
    inline QString command() const
    {
        return m_sCommand;
    }

    //=========================================================================================================
    /**
    * Returns the number of parameters.
    *
    * @return number of parameters.
    */
    inline quint32 count() const
    {
        return m_mapParameters.size();
    }

    //=========================================================================================================
    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    inline QString description() const
    {
        return m_sDescription;
    }




    virtual void execute()
    {
        //ToDo emit triggered
        return;
    }

    //=========================================================================================================
    /**
    * If received command was Json fomratted or triggered command should be Json formatted.
    *
    * @return Json formatted.
    */
    inline bool& isJson()
    {
        return m_bIsJson;
    }

    //=========================================================================================================
    /**
    * Get parameter descriptions
    *
    * @return parameter descriptions
    */
    inline QList<QString> pDescriptions() const
    {
        return m_vecParamDescriptions;
    }

    //=========================================================================================================
    /**
    * Get parameter names
    *
    * @return parameter names
    */
    inline QList<QString> pNames() const
    {
        return m_mapParameters.keys();
    }

    //=========================================================================================================
    /**
    * Returns parameter values
    *
    * @return parameter values
    */
    inline QList<QVariant> pValues() const
    {
        return m_mapParameters.values();
    }

    //=========================================================================================================
    /**
    * Receiver slot which performs parameter check before the received signal is emmited
    * If parameter check is passed, values are assigned to this object instance.
    *
    * @param p_Command  Command which was received and has to be checked before it's emmited.
    */
    void verify(const Command &p_Command);

    //=========================================================================================================
    /**
    * Sender slot which performs parameter check before the triggered signal is emmited
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
    * Assignment Operator
    *
    * @param[in] rhs     Command which should be assigned.
    */
    Command& operator= (const Command &rhs);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the parameter name.
    *
    * @return Parameter value related to the parameter name.
    */
    QVariant& operator[] (const QString &key);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the parameter name.
    *
    * @return Parameter value related to the parameter name.
    */
    const QVariant operator[] (const QString &key) const;

signals:
    void triggered(Command);
    void received(Command);

public:
    bool                m_bIsJson;
    QString             m_sCommand;
    QString             m_sDescription;
    QMap<QString, QVariant> m_mapParameters;
    QList<QString>      m_vecParamDescriptions;
};

} // NAMESPACE

#endif // COMMAND_H
