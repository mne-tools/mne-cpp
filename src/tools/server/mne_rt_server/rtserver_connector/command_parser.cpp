//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command_parser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Translation unit for @ref RTSERVER::CommandParser: JSON/CLI tokenisation and fan-out to attached managers.
 *
 * Implements the dual-dialect @c parse() entry point. CLI input is
 * split on whitespace into a keyword and string arguments; JSON input
 * is decoded with @c QJsonDocument and may contain either a single
 * command object or an array of them, supporting batched requests in a
 * single payload. In either case the result is a populated
 * @ref RawCommand handed to every attached @ref CommandManager via the
 * @c Subject::notify mechanism inherited from @ref RTSERVER::Subject; the
 * @c response signal carries replies produced by handlers back to the
 * originating client of the command port.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "command_parser.h"
#include <base/log.h>
#include "command_manager.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandParser::CommandParser(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

bool CommandParser::exists(const QString& p_sCommand)
{
    for (IObserver* observer : observers()) {
        CommandManager* t_pCommandManager = static_cast<CommandManager*>(observer);
        if (t_pCommandManager->hasCommand(p_sCommand))
            return true;
    }
    return false;
}

//=============================================================================================================

bool CommandParser::parse(const QString& p_sInput, QStringList& p_qListCommandsParsed)
{
    if (p_sInput.size() <= 0)
        return false;

    p_qListCommandsParsed.clear();

    //Check if JSON format;
    bool isJson = false;
    if (QString::compare(p_sInput.at(0), QString("{")) == 0)
        isJson = true;

    if (isJson) {
        mne::Log::debug("JSON command recognized");

        QJsonObject t_jsonObjectCommand;
        QJsonObject t_jsonObjectParameters;
        QJsonDocument t_jsonDocument(QJsonDocument::fromJson(p_sInput.toUtf8()));

        //Switch to command object
        if (t_jsonDocument.isObject() && t_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
            t_jsonObjectCommand = t_jsonDocument.object().value(QString("commands")).toObject();
        else
            return false;

        //iterate over commands
        QJsonObject::Iterator it;
        QJsonObject::Iterator itParam;
        for (it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it) {
            //Print Command
            mne::Log::info("{}\r\n", it.key().toStdString());

            if (exists(it.key())) {
                RawCommand t_rawCommand(it.key(), true);
                m_rawCommand = t_rawCommand;
                t_jsonObjectParameters = it.value().toObject();

                // push command to processed commands
                p_qListCommandsParsed.push_back(it.key());

                //append the parameters
                for (itParam = t_jsonObjectParameters.begin(); itParam != t_jsonObjectParameters.end(); ++itParam) {
                    mne::Log::info(" {}", itParam.value().toString().toStdString());
                    //ToDo do a cross check with the param naming and key
                    m_rawCommand.pValues().append(itParam.value().toString());
                }

                //Notify attached command manager
                notify();
            }
            mne::Log::info("\r\n");
        }
    } else {
        QStringList t_qCommandList = p_sInput.split(" ");

        //Print command
        mne::Log::info("{}\r\n", t_qCommandList[0].toStdString());

        if (!exists(t_qCommandList[0])) {
            mne::Log::info("\r\n");
            return false;
        }

        RawCommand t_rawCommand(t_qCommandList[0], false);
        m_rawCommand = t_rawCommand;

        // push command to processed commands
        p_qListCommandsParsed.push_back(t_qCommandList[0]);

        if (t_qCommandList.size() > 1) //Parameter parsing
        {
            //Parse Parameters
            for (qint32 i = 1; i < t_qCommandList.size(); ++i) {
                mne::Log::info(" {}", t_qCommandList[i].toStdString());
                m_rawCommand.pValues().append(t_qCommandList[i]);
            }
        }
        mne::Log::info("\r\n");
        notify();
    }

    return true;
}
