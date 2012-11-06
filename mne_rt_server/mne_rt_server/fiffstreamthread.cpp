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
#include "fiffstreamserver.h"


#include "mne_rt_commands.h"

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

FiffStreamThread::FiffStreamThread(qint32 id, int socketDescriptor, QObject *parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, socketDescriptor(socketDescriptor)
{
}


//*************************************************************************************************************

FiffStreamThread::~FiffStreamThread()
{
    //Remove from client list
    FiffStreamServer* t_pFiffStreamServer = qobject_cast<FiffStreamServer*>(this->parent());
    t_pFiffStreamServer->m_qClientList.remove(m_iDataClientId);
}


//*************************************************************************************************************

void FiffStreamThread::parseCommand(FiffTag* p_pTag)
{
    if(p_pTag->size() >= 4)
    {
        qint32* t_pInt = (qint32*)p_pTag->data();
        FiffTag::swap_intp(t_pInt);
        qint32 t_iCmd = t_pInt[0];

        if(t_iCmd == MNE_RT_SET_CLIENT_ALIAS)
        {
            //
            // Set Client Alias
            //
            m_sDataClientAlias = QString(p_pTag->mid(4, p_pTag->size()-4));
            printf("FiffStreamClient (ID %d): new alias = '%s'\r\n\n", m_iDataClientId, m_sDataClientAlias.toUtf8().constData());
        }
        else if(t_iCmd == MNE_RT_GET_CLIENT_ID)
        {
            //
            // Send Client ID
            //
            printf("FiffStreamClient (ID %d): send client ID %d\r\n\n", m_iDataClientId, m_iDataClientId);
            writeClientId();
        }
        else
        {
            printf("FiffStreamClient (ID %d): unknown command\r\n\n", m_iDataClientId);
        }
    }
    else
    {
        printf("FiffStreamClient (ID %d): unknown command\r\n\n", m_iDataClientId);
    }
}


//*************************************************************************************************************

void FiffStreamThread::getAndSendMeasurementInfo(qint32 ID, FiffInfo* p_pFiffInfo)
{
    if(ID == m_iDataClientId)
    {
        mutex.lock();
        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
        t_FiffStreamOut.setVersion(QDataStream::Qt_5_0);

        qint32 init_info[2];
        init_info[0] = FIFF_MNE_RT_CLIENT_ID;
        init_info[1] = m_iDataClientId;

        t_FiffStreamOut.start_block(FIFFB_MNE_RT_MEAS_INFO);
        mutex.unlock();
    }
}


//*************************************************************************************************************

void FiffStreamThread::writeClientId()
{
    FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
    t_FiffStreamOut.setVersion(QDataStream::Qt_5_0);

    t_FiffStreamOut.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId);
}


//*************************************************************************************************************

void FiffStreamThread::run()
{
    FiffStreamServer* parentServer = qobject_cast<FiffStreamServer*>(this->parent());
//    connect(parentServer, &FiffStreamServer::sendFiffStreamThreadInstruction,
//            this, &FiffStreamThread::readFiffStreamServerInstruction);

    connect(parentServer, &FiffStreamServer::sendMeasInfo,
            this, &FiffStreamThread::getAndSendMeasurementInfo);


    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    }
    else
    {
        printf("FiffStreamClient (assigned ID %d) accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               m_iDataClientId,
               QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket.peerPort());
    }


    FiffStream t_FiffStreamIn(&t_qTcpSocket);
    t_FiffStreamIn.setVersion(QDataStream::Qt_5_0);

    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
    {
        //
        // Write available data
        //
        if(m_qSendBlock.size() > 0)
        {
//            qDebug() << "data available" << m_qSendBlock.size();
            mutex.lock();
            t_qTcpSocket.write(m_qSendBlock);
            t_qTcpSocket.waitForBytesWritten();
            m_qSendBlock.clear();
            mutex.unlock();
        }

        //
        // Wait 10ms for incomming tag header, read and continue
        //
        t_qTcpSocket.waitForReadyRead(10);

        if (t_qTcpSocket.bytesAvailable() >= (int)sizeof(qint32)*4)
        {
            FiffTag* t_pTag = NULL;
            FiffTag::read_tag_info(&t_FiffStreamIn, t_pTag, false);

            //
            // wait until tag size data are available and read the data
            //
            while (t_qTcpSocket.bytesAvailable() < t_pTag->size())
            {
                t_qTcpSocket.waitForReadyRead(10);
            }
            FiffTag::read_tag_data(&t_FiffStreamIn, t_pTag);

            //
            // Parse the tag
            //
            if(t_pTag->kind == FIFF_MNE_RT_COMMAND)
            {
                parseCommand(t_pTag);
            }

        }

//        usleep(1000);
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
