//=============================================================================================================
/**
 * @file     command_manager.cpp
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
 * @brief    Definition of the CommandManager Class.
 *
 */

//=============================================================================================================
// Includes
//=============================================================================================================

#include "command_manager.h"
#include "raw_command.h"

//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QVector>
#include <QDebug>
#include <QJsonObject>
#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandManager::CommandManager(bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
{
    init();
}

//=============================================================================================================

CommandManager::CommandManager(const QByteArray &p_qByteArrayJsonDoc, bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
{
    init();

    m_jsonDocumentOrigin = QJsonDocument::fromJson(p_qByteArrayJsonDoc);

    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::CommandManager(const QJsonDocument &p_jsonDoc, bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
, m_jsonDocumentOrigin(p_jsonDoc)
{
    init();

    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::~CommandManager()
{
    //Disconnect all connections which are created with the help of this manager.
//    this->disconnectAll();

    //Remove commands which where inserted into the static command list
}

//=============================================================================================================

void CommandManager::clear()
{
    m_qMapCommands.clear();
}

//=============================================================================================================

void CommandManager::init()
{
}

//=============================================================================================================
//ToDo connect all commands inserted in this class by default.
void CommandManager::insert(const QJsonDocument &p_jsonDocument)
{
    QJsonObject t_jsonObjectCommand;

    //Switch to command object
    if(p_jsonDocument.isObject() && p_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
        t_jsonObjectCommand = p_jsonDocument.object().value(QString("commands")).toObject();
    else
        return;

    QJsonObject::Iterator it;
    for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
    {
        if(!m_qMapCommands.contains(it.key()))
            m_qMapCommands.insert(it.key(), Command(it.key(), it.value().toObject(), true, this));
        else
            qWarning("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toUtf8().constData());
    }

    emit commandMapChanged();
}

//=============================================================================================================

void CommandManager::insert(const QString &p_sKey, const QString &p_sDescription)
{
    Command t_command(p_sKey, p_sDescription, false, this);
    insert(p_sKey, t_command);
}

//=============================================================================================================

void CommandManager::insert(const QString &p_sKey, const Command &p_command)
{
    Command t_command(p_command);
    t_command.setParent(this);
    m_qMapCommands.insert(p_sKey, t_command);
    emit commandMapChanged();
}

//=============================================================================================================

void CommandManager::update(UTILSLIB::Subject* p_pSubject)
{
    // If Manager is not active do not parse commands
    if(!m_bIsActive)
        return;

    CommandParser* t_pCommandParser = static_cast<CommandParser*>(p_pSubject);

    RawCommand t_rawCommand(t_pCommandParser->getRawCommand());
    QString t_sCommandName = t_rawCommand.command();

    if(!this->hasCommand(t_sCommandName))
        return;

    // check if number of parameters is right
    if(t_rawCommand.count() >= m_qMapCommands[t_sCommandName].count())
    {
        m_qMapCommands[t_sCommandName].isJson() = t_rawCommand.isJson();

        //Parse Parameters
        for(quint32 i = 0; i < m_qMapCommands[t_sCommandName].count(); ++i)
        {
            QMetaType t_metaType = m_qMapCommands[t_sCommandName][i].metaType();

            QVariant t_qVariantParam(t_rawCommand.pValues()[i]);

            if(t_qVariantParam.canConvert(t_metaType) && t_qVariantParam.convert(t_metaType))
                m_qMapCommands[t_sCommandName][i] = t_qVariantParam;
            else
                return;
        }

        m_qMapCommands[t_sCommandName].execute();
    }
}

//=============================================================================================================

Command& CommandManager::operator[] (const QString &key)
{
    return m_qMapCommands[key];
}

//=============================================================================================================

const Command CommandManager::operator[] (const QString &key) const
{
    return m_qMapCommands[key];
}
