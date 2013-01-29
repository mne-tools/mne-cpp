//=============================================================================================================
/**
* @file     commandparser.h
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
* @brief    Declaration of the CommandParser Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandparser.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCOMMANDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandParser::CommandParser(QObject *parent)
: QObject(parent)
{
}


//*************************************************************************************************************

bool CommandParser::parse(const QString &p_sInput)
{
    if(p_sInput.size() <= 0)
        return false;
    //Check if JSON format;
    bool isJson  = false;
    if(QString::compare(p_sInput.at(0), QString("{")) == 0)
        isJson = true;

    if(isJson)
    {
//        RawCommand parsedCommand;
        qDebug() << "JSON commands recognized";

        QJsonObject t_jsonObjectCommand;
        QJsonObject t_jsonObjectParameters;
        QJsonDocument t_jsonDocument(QJsonDocument::fromJson(p_sInput.toLatin1()));

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
            m_rawCommand = RawCommand(it.key(), true);
            t_jsonObjectParameters = it.value().toObject();

            //append the parameters
            for(itParam= t_jsonObjectParameters.begin(); itParam != t_jsonObjectParameters.end(); ++itParam)
            {
                //ToDo do a cross check with the param naming and key
                m_rawCommand.pValues().append(itParam.value().toString());
//                qDebug() << itParam.key() << " + " << itParam.value().toString();
            }
            notify();
        }
    }
    else
    {
        QStringList t_qCommandList = p_sInput.split(" ");

        m_rawCommand = RawCommand(t_qCommandList[0], false);

        if(t_qCommandList.size() > 1) //Parameter parsing
        {
            //Parse Parameters
            for(qint32 i = 1; i < t_qCommandList.size(); ++i)
                m_rawCommand.pValues().append(t_qCommandList[i]);
        }

        notify();
    }

    return true;
}
