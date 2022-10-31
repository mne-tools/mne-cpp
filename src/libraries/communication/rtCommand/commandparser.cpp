//=============================================================================================================
/**
 * @file     commandparser.cpp
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandparser.h"
#include "commandmanager.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandParser::CommandParser(QObject *parent)
: QObject(parent)
{
}

//=============================================================================================================

bool CommandParser::exists(const QString& p_sCommand)
{
    Subject::t_Observers::Iterator itObservers;
    for(itObservers = this->observers().begin(); itObservers != this->observers().end(); ++itObservers)
    {
        CommandManager* t_pCommandManager = static_cast<CommandManager*> (*itObservers);
        if(t_pCommandManager->hasCommand(p_sCommand))
            return true;
    }
    return false;
}

//=============================================================================================================

bool CommandParser::parse(const QString &p_sInput, QStringList &p_qListCommandsParsed)
{
    if(p_sInput.size() <= 0)
        return false;

    p_qListCommandsParsed.clear();

    //Check if JSON format;
    bool isJson  = false;
    if(QString::compare(p_sInput.at(0), QString("{")) == 0)
        isJson = true;

    if(isJson)
    {
        qDebug() << "JSON command recognized";

        QJsonObject t_jsonObjectCommand;
        QJsonObject t_jsonObjectParameters;
        QJsonDocument t_jsonDocument(QJsonDocument::fromJson(p_sInput.toUtf8()));

        //Switch to command object
        if(t_jsonDocument.isObject() && t_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
            t_jsonObjectCommand = t_jsonDocument.object().value(QString("commands")).toObject();
        else
            return false;

        //iterate over commands
        QJsonObject::Iterator it;
        QJsonObject::Iterator itParam;
        for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
        {
            //Print Command
            printf("%s\r\n", it.key().toUtf8().constData());

            if(exists(it.key()))
            {
                RawCommand t_rawCommand(it.key(), true);
                m_rawCommand = t_rawCommand;
                t_jsonObjectParameters = it.value().toObject();

                // push command to processed commands
                p_qListCommandsParsed.push_back(it.key());

                //append the parameters
                for(itParam= t_jsonObjectParameters.begin(); itParam != t_jsonObjectParameters.end(); ++itParam)
                {
                    printf(" %s", itParam.value().toString().toUtf8().constData());
                    //ToDo do a cross check with the param naming and key
                    m_rawCommand.pValues().append(itParam.value().toString());
//                    qDebug() << itParam.key() << " + " << itParam.value().toString();
                }

                //Notify attached command manager
                notify();
            }
            printf("\r\n");
        }
    }
    else
    {
        QStringList t_qCommandList = p_sInput.split(" ");

        //Print command
        printf("%s\r\n", t_qCommandList[0].toUtf8().constData());

        if(!exists(t_qCommandList[0]))
        {
            printf("\r\n");
            return false;
        }

        RawCommand t_rawCommand(t_qCommandList[0], false);
        m_rawCommand = t_rawCommand;

        // push command to processed commands
        p_qListCommandsParsed.push_back(t_qCommandList[0]);

        if(t_qCommandList.size() > 1) //Parameter parsing
        {
            //Parse Parameters
            for(qint32 i = 1; i < t_qCommandList.size(); ++i)
            {
                printf(" %s", t_qCommandList[i].toUtf8().constData());
                m_rawCommand.pValues().append(t_qCommandList[i]);
            }
        }
        printf("\r\n");
        notify();
    }

    return true;
}
