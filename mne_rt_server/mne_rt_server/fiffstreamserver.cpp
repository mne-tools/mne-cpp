//=============================================================================================================
/**
* @file     fiffstreamserver.cpp
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
* @brief    Contains the implementation of the FiffStreamServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"


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
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamServer::FiffStreamServer(QObject *parent)
: QTcpServer(parent)
, m_iNextClientId(0)
{

}


//*************************************************************************************************************

FiffStreamServer::~FiffStreamServer()
{
    clearClients();
}


//*************************************************************************************************************

void FiffStreamServer::clearClients()
{
    QMap<qint32, FiffStreamThread*>::const_iterator i = m_qClientList.constBegin();
    while (i != m_qClientList.constEnd()) {
        if(i.value())
            delete i.value();
        ++i;
    }
    m_qClientList.clear();
}


//*************************************************************************************************************

void FiffStreamServer::forwardActivateRawDataFiffStreamClient(qint32 ID)
{
    emit activateRawDataFiffStreamClient(ID);
}


//*************************************************************************************************************

void FiffStreamServer::forwardMeasInfo(qint32 ID, FiffInfo* p_pFiffInfo)
{
    emit remitMeasInfo(ID, p_pFiffInfo);
}


//*************************************************************************************************************
//ToDo increase preformance --> try inline
void FiffStreamServer::forwardRawBuffer(Eigen::MatrixXf m_matRawData)
{
    emit remitRawBuffer(m_matRawData);
}


//*************************************************************************************************************

void FiffStreamServer::incomingConnection(qintptr socketDescriptor)
{
    FiffStreamThread* streamThread = new FiffStreamThread(m_iNextClientId, socketDescriptor, this);

    m_qClientList.insert(m_iNextClientId, streamThread);
    ++m_iNextClientId;

    //when thread has finished it gets deleted
    connect(streamThread, SIGNAL(finished()), streamThread, SLOT(deleteLater()));

    streamThread->start();
}
