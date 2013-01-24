//=============================================================================================================
/**
* @file     commandserver.h
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
* @brief     implementation of the CommandServer Class.
*
*/

#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ICommandParser.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QTcpServer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MSERVER
//=============================================================================================================

namespace MSERVER
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//=============================================================================================================
/**
* DECLARE CLASS CommandServer
*
* @brief The CommandServer class provides
*/
class CommandServer : public QTcpServer, ICommandParser
{
    Q_OBJECT

public:
    CommandServer(QObject *parent = 0);

    ~CommandServer();

    virtual QByteArray availableCommands();

    void incommingCommand(QString p_sCommand, qint32 p_iThreadID);

    virtual bool parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo);

    void registerCommandParser(ICommandParser* p_pCommandParser);

signals:
    void replyCommand(QByteArray p_blockReply, qint32 p_iID);

//    void stopMeasConnector();
//    void startMeasFiffStreamClient(qint32 ID);
    void closeCommandThreads();

protected:
    void incomingConnection(qintptr socketDescriptor);

private:

    QList<ICommandParser*> m_qListParser;

    qint32 m_iThreadCount;

};

} // NAMESPACE

#endif //INSTRUCTIONSERVER_H
