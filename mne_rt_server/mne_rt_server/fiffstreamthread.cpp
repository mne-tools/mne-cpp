//=============================================================================================================
/**
* @file     fiffstreamthread.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    Contains the implementation of the FiffStreamThread Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"


#include "mne_rt_constants.h"

#include "../../MNE/fiff/fiff_stream.h"
#include "../../MNE/fiff/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>


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

FiffStreamThread::FiffStreamThread(quint8 id, int socketDescriptor, QObject *parent)
: QThread(parent)
, m_iClientId(id)
, socketDescriptor(socketDescriptor)
{
}


//*************************************************************************************************************

void FiffStreamThread::write_client_info(QTcpSocket& p_qTcpSocket)
{

    QByteArray block;

    FiffStream t_FiffStream(&block, QIODevice::WriteOnly);
    t_FiffStream.setVersion(QDataStream::Qt_5_0);

    qint32 init_info[2];
    init_info[0] = MNE_RT_CLIENT_ID;
    init_info[1] = m_iClientId;

    t_FiffStream.write_int(FIFF_MNE_RT_COMMAND, init_info, 2);

    p_qTcpSocket.write(block);
    p_qTcpSocket.flush();

    block.clear();
}


//*************************************************************************************************************

void FiffStreamThread::run()
{
    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    }
    else
    {
        printf("Fiff stream client connection accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket.peerPort());
    }



    write_client_info(t_qTcpSocket);

//    QByteArray block;

//    FiffStream t_FiffStream(&block, QIODevice::WriteOnly);
//    t_FiffStream.setVersion(QDataStream::Qt_5_0);

//    qint32 init_info[2];
//    init_info[0] = MNE_RT_CLIENT_ID;
//    init_info[1] = m_iClientId;

//    t_FiffStream.write_int(FIFF_MNE_RT_COMMAND, init_info, 2);

//    t_qTcpSocket.write(block);
//    t_qTcpSocket.flush();

//    block.clear();




//    tcpSocket.disconnectFromHost();
//    tcpSocket.waitForDisconnected();

//    while(true)
//    {

//    }
}
