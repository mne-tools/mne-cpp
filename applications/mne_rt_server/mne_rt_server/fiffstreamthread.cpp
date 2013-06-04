//=============================================================================================================
/**
* @file     fiffstreamthread.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>
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
* @brief     implementation of the FiffStreamThread Class.
*
*/

//*
//* May, 2013 modified by Dr. -Ing. Limin Sun
//* Changes : The socket pointor was passed from the top level
//*

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"
#include "fiffstreamserver.h"
#include "mne_rt_commands.h"


//*************************************************************************************************************
//=============================================================================================================
// Fiff INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace RTSERVER;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamThread::FiffStreamThread(qint32 id, QTcpSocket *socket, QObject *parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, m_bIsSendingRawBuffer(false)
, t_qTcpSocket(socket)
, m_bIsRunning(false)
{
}


//*************************************************************************************************************

FiffStreamThread::~FiffStreamThread()
{
    //Remove from client list
    FiffStreamServer* t_pFiffStreamServer = qobject_cast<FiffStreamServer*>(this->parent());
    if(t_pFiffStreamServer)
        t_pFiffStreamServer->m_qClientList.remove(m_iDataClientId);

    m_bIsRunning = false;
    QThread::wait();
    delete t_qTcpSocket;
}


//*************************************************************************************************************

void FiffStreamThread::startMeas(qint32 ID)
{
    if(ID == m_iDataClientId)
    {
        qDebug() << "Activate raw buffer sending.";

        m_qMutex.lock();
        // ToDo send start meas
        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
        t_FiffStreamOut.start_block(FIFFB_RAW_DATA);
        m_bIsSendingRawBuffer = true;
        m_qMutex.unlock();
    }
}


//*************************************************************************************************************

void FiffStreamThread::stopMeas(qint32 ID)
{
    qDebug() << "void FiffStreamThread::stopMeas(qint32 ID)";
    if(ID == m_iDataClientId || ID == -1)
    {
        qDebug() << "stop raw buffer sending.";

        m_qMutex.lock();
        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
        t_FiffStreamOut.end_block(FIFFB_RAW_DATA);
        m_bIsSendingRawBuffer = false;
        m_qMutex.unlock();
    }

}


//*************************************************************************************************************

void FiffStreamThread::parseCommand(FiffTag::SPtr p_pTag)
{
    if(p_pTag->size() >= 4)
    {
        qint32* t_pInt = (qint32*)p_pTag->data();
        IOUtils::swap_intp(t_pInt);
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

void FiffStreamThread::sendRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData)
{
    if(m_bIsSendingRawBuffer)
    {
//        qDebug() << "Send RawBuffer to client";

        m_qMutex.lock();

        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
        t_FiffStreamOut.write_float(FIFF_DATA_BUFFER,m_pMatRawData->data(),m_pMatRawData->rows()*m_pMatRawData->cols());
        m_qMutex.unlock();

    }
//    else
//    {
//        qDebug() << "Send RawBuffer is not activated";
//    }
}

void FiffStreamThread::send_data()
{
    if(t_qTcpSocket->state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {
        m_qMutex.lock();
        qint32 t_iBlockSize = m_qSendBlock.size();
        if(t_iBlockSize > 0)
        {
//            qDebug() << "data available" << t_iBlockSize;
            qint32 t_iBytesWritten = t_qTcpSocket->write(m_qSendBlock);
            qDebug()<<"[Block to write]"<<t_iBlockSize<<"[bytes has been Written]"<<t_iBytesWritten;
            t_qTcpSocket->waitForBytesWritten();
        }
        m_qMutex.unlock();
    }
}

//*************************************************************************************************************

void FiffStreamThread::sendMeasurementInfo(qint32 ID, FiffInfo p_fiffInfo)
{
    if(ID == m_iDataClientId)
    {
        m_qMutex.lock();


        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);

//        qint32 init_info[2];
//        init_info[0] = FIFF_MNE_RT_CLIENT_ID;
//        init_info[1] = m_iDataClientId;

//        t_FiffStreamOut.start_block(FIFFB_MNE_RT_MEAS_INFO);

//FiffStream::start_writing_raw

        p_fiffInfo.writeToStream(&t_FiffStreamOut);
        m_qMutex.unlock();

//        qDebug() << "MeasInfo Blocksize: " << m_qSendBlock.size();
    }
}


//*************************************************************************************************************

void FiffStreamThread::writeClientId()
{
    FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);

    t_FiffStreamOut.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId);
}


void FiffStreamThread::ReadProc()
{
    FiffStream t_FiffStreamIn(t_qTcpSocket);

    //
    // Read: Wait 10ms for incomming tag header, read and continue
    //
    t_qTcpSocket->waitForReadyRead(10);

    if (t_qTcpSocket->bytesAvailable() >= (int)sizeof(qint32)*4)
    {
        qDebug() << "goes to read bytes " ;
        FiffTag::SPtr t_pTag;
        FiffTag::read_tag_info(&t_FiffStreamIn, t_pTag, false);

        //
        // wait until tag size data are available and read the data
        //
        while (t_qTcpSocket->bytesAvailable() < t_pTag->size())
        {
            t_qTcpSocket->waitForReadyRead(10);
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
}

//*************************************************************************************************************

void FiffStreamThread::run()
{
    m_bIsRunning = true;

    printf("FiffStreamClient (assigned ID %d) accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               m_iDataClientId,
               QHostAddress(t_qTcpSocket->peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket->peerPort());

    FiffStream t_FiffStreamIn(t_qTcpSocket);

    int i=0;
    while(t_qTcpSocket->state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {
        //
        // Write available data
        //
        m_qMutex.lock();
        qint32 t_iBlockSize = m_qSendBlock.size();
        if(t_iBlockSize > 0)
        {
//            qDebug() << "data available" << t_iBlockSize;
            qint32 t_iBytesWritten = t_qTcpSocket->write(m_qSendBlock);
            qDebug()<<++i<<"[Block to write]"<<t_iBlockSize<<"[bytes has been Written]"<<t_iBytesWritten;
            t_qTcpSocket->waitForBytesWritten();

            if (t_iBytesWritten < t_iBlockSize)
            {
                qDebug()<<i<<"[bytes alreadey been Written]"<<t_iBytesWritten;

                qint32 size_w = t_qTcpSocket->bytesToWrite();
                for(;;)
                {
                    qDebug()<<++i<<"[waited bytes to Write]"<<size_w;

                    t_qTcpSocket->waitForBytesWritten(1000);
                    size_w = t_qTcpSocket->bytesToWrite();
                    if (size_w == 0) break;
                }

            }
//            if(t_iBytesWritten == t_iBlockSize)
//            {
                m_qSendBlock.clear();
//            }
//            else
//            {
//                m_qSendBlock = m_qSendBlock.mid(t_iBytesWritten, t_iBlockSize-t_iBytesWritten);
//            }
        }
        m_qMutex.unlock();

        //
        // Read: Wait 10ms for incomming tag header, read and continue
        //
        t_qTcpSocket->waitForReadyRead(10);

        if (t_qTcpSocket->bytesAvailable() >= (int)sizeof(qint32)*4)
        {
            qDebug() << "goes to read bytes " ;
            FiffTag::SPtr t_pTag;
            FiffTag::read_tag_info(&t_FiffStreamIn, t_pTag, false);

            //
            // wait until tag size data are available and read the data
            //
            while (t_qTcpSocket->bytesAvailable() < t_pTag->size())
            {
                t_qTcpSocket->waitForReadyRead(10);
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
    }

    t_qTcpSocket->disconnectFromHost();
    if(t_qTcpSocket->state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket->waitForDisconnected();
}
