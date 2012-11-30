//=============================================================================================================
/**
* @file     commandserver.cpp
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
* @brief    Contains the implementation of the CommandServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandserver.h"
#include "commandthread.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdlib.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandServer::CommandServer(QObject *parent)
: QTcpServer(parent)
{

}


//*************************************************************************************************************

CommandServer::~CommandServer()
{
    emit closeCommandThreads();
}


//*************************************************************************************************************

void CommandServer::incomingConnection(qintptr socketDescriptor)
{
    CommandThread* t_pCommandThread = new CommandThread(socketDescriptor, this);

    //when thread has finished it gets deleted
    connect(t_pCommandThread, SIGNAL(finished()), t_pCommandThread, SLOT(deleteLater()));
    connect(this, SIGNAL(closeCommandThreads()), t_pCommandThread, SLOT(deleteLater()));

    //Forwards for thread safety - check if obsolete!?
    connect(t_pCommandThread, &CommandThread::requestMeasInfo,
            this, &CommandServer::forwardMeasInfo);
    connect(t_pCommandThread, &CommandThread::requestStartMeas,
            this, &CommandServer::forwardStartMeas);
    connect(t_pCommandThread, &CommandThread::requestStopMeas,
            this, &CommandServer::forwardStopMeas);
    connect(t_pCommandThread, &CommandThread::requestStopConnector,
            this, &CommandServer::forwardStopConnector);
//    connect(t_pCommandThread, &CommandThread::requestSetBufferSize,
//            this, &CommandServer::forwardSetBufferSize);

    t_pCommandThread->start();
}


//*************************************************************************************************************

void CommandServer::forwardMeasInfo(qint32 ID)
{
    emit requestMeasInfoConnector(ID);
}


//*************************************************************************************************************

void CommandServer::forwardStartMeas(qint32 ID)
{
    emit startMeasFiffStreamClient(ID);
    emit startMeasConnector();
}


//*************************************************************************************************************

void CommandServer::forwardStopMeas(qint32 ID)
{
    emit stopMeasFiffStreamClient(ID);
}


//*************************************************************************************************************

void CommandServer::forwardStopConnector()
{
    emit stopMeasConnector();
}


////*************************************************************************************************************

//void CommandServer::forwardSetBufferSize(quint32 p_uiBuffSize)
//{
//    emit setBufferSize(p_uiBuffSize);
//}

