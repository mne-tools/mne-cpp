//=============================================================================================================
/**
 * @file     command_manager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    Declaration of the CommandManager Class.
 *
 */

#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//=============================================================================================================
// Includes
//=============================================================================================================

#include "../com_global.h"
#include "command.h"
#include "command_parser.h"

#include <utils/generics/observerpattern.h>

//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QObject>
#include <QJsonDocument>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Registry of available commands; dispatches parsed command strings to the matching Command handler
 */
class COMSHARED_EXPORT CommandManager : public QObject, public UTILSLIB::IObserver
{
    Q_OBJECT

public:
    explicit CommandManager(bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QByteArray &p_qByteArrayJsonDoc, bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QJsonDocument &p_jsonDoc, bool p_bIsActive = true, QObject *parent = 0);

    virtual ~CommandManager();

    //=========================================================================================================
    /**
     * Clears the command manager
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns the lookup table of all available commands.
     *
     * @return the command lookup table.
     */
    inline QMap<QString, Command>& commandMap();

    //=========================================================================================================
    /**
     * Checks if a command is managed;
     *
     * @param p_sCommand     COmmand to check.
     *
     * @return true if part of command manager, false otherwise.
     */
    inline bool hasCommand(const QString &p_sCommand) const;

    //=========================================================================================================
    /**
     * Inserts commands encoded in a json document.
     * Attention existing items are overwritten.
     *
     * @param p_jsonDocument    JSON document containing commands.
     */
    void insert(const QJsonDocument &p_jsonDocument);

    //=========================================================================================================
    /**
     * Inserts a single command.
     * Attention existing items are overwritten.
     *
     * @param p_jsonDocument    JSON document containing commands.
     */
    void insert(const QString &p_sKey, const QString &p_sDescription);

    //=========================================================================================================
    /**
     * Inserts a new command and emmits dataChanged signal.
     *
     * @param p_sKey     Command key word.
     * @param p_command  Command content. Attention CommandManager takes ownership of that command by reseting commad's parent;.
     */
    void insert(const QString &p_sKey, const Command &p_command);

    //=========================================================================================================
    /**
     * Returns if CommandManager is active. If true, this manager parses incomming commands.
     *
     * @return if true, incomming commands are parsed.
     */
    inline bool isActive() const;

    //=========================================================================================================
    /**
     * Sets the activation status of the CommandManager.
     *
     * @param[in] status the new activation status of the CommandManager.
     */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
     * Updates the IObserver (CommandManager) when a new command was received.
     *
     * @param[in] p_pSubject  pointer to the subject (CommandParser) to which observer (CommandManager) is attached to.
     */
    virtual void update(UTILSLIB::Subject* p_pSubject);

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param[in] key    the command key word.
     *
     * @return Command object related to command key word.
     */
    Command& operator[] (const QString &key);

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param key    the command key word.
     *
     * @return Command object related to command key word.
     */
    const Command operator[] (const QString &key) const;

private:
    //=========================================================================================================
    /**
     * Initializes the command manager by connecting internal signal/slots
     */
    void init();

    bool m_bIsActive;

    QJsonDocument m_jsonDocumentOrigin;

    QMetaObject::Connection m_conReplyChannel;      /**< The reply channel of the command manager. */

    QMap<QString, Command> m_qMapCommands;          /**< Holds a map as an internal lookuptable of available commands. */

signals:
    void commandMapChanged();//(QStringList)

    //=========================================================================================================
    /**
     * Is emitted when a command is ready to send
     *
     * @param p_command  Command which should be send.
     */
    void triggered(Command p_command);

    //=========================================================================================================
    /**
     * Is triggered when a reply is available. Commands are the emmiters of this signal -> access trough parent.
     *
     * @param p_sReply   the plain or JSON formatted reply.
     *@param p_command   Command which send the response
     */
    void response(QString p_sReply, Command p_command);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QMap<QString, Command>& CommandManager::commandMap()
{
    return m_qMapCommands;
}

//=============================================================================================================

inline bool CommandManager::hasCommand(const QString &p_sCommand) const
{
    return m_qMapCommands.contains(p_sCommand);
}

//=============================================================================================================

inline bool CommandManager::isActive() const
{
    return m_bIsActive;
}

//=============================================================================================================

inline void CommandManager::setStatus(bool status)
{
    m_bIsActive = status;
}
} // NAMESPACE

#endif // COMMUNICATIONMANAGER_H
