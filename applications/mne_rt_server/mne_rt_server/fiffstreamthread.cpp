//=============================================================================================================
/**
 * @file     fiffstreamthread.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 *
 *                      Copyright (C) 2012, Christoph Dinh, Felix Arndt, Limin Sun, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     Definition of the FiffStreamThread Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"
#include "fiffstreamserver.h"
#include "mne_rt_commands.h"

//=============================================================================================================
// Fiff INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace RTSERVER;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamThread::FiffStreamThread(qint32 id, int socketDescriptor, QObject *parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, m_iSocketDescriptor(socketDescriptor)
, m_bIsSendingRawBuffer(false)
, m_bIsRunning(false)
{
}

//=============================================================================================================

FiffStreamThread::~FiffStreamThread()
{
    //Remove from client list
    FiffStreamServer* t_pFiffStreamServer = qobject_cast<FiffStreamServer*>(this->parent());
    if(t_pFiffStreamServer)
        t_pFiffStreamServer->m_qClientList.remove(m_iDataClientId);

    m_bIsRunning = false;
    QThread::wait();
}

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

//void FiffStreamThread::sendData(QTcpSocket& p_qTcpSocket)
//{
//    if(p_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
//    {
//        m_qMutex.lock();
//        qint32 t_iBlockSize = m_qSendBlock.size();
//        if(t_iBlockSize > 0)
//        {
////            qDebug() << "data available" << t_iBlockSize;
//            qint32 t_iBytesWritten = p_qTcpSocket.write(m_qSendBlock);
//            qDebug()<<"[Block to write]"<<t_iBlockSize<<"[bytes has been Written]"<<t_iBytesWritten;
//            p_qTcpSocket.waitForBytesWritten();
//        }
//        m_qMutex.unlock();
//    }
//}

//=============================================================================================================

void FiffStreamThread::sendMeasurementInfo(qint32 ID, const FiffInfo& p_fiffInfo)
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

//=============================================================================================================

void FiffStreamThread::writeClientId()
{
    FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);

    t_FiffStreamOut.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId);
}

//=============================================================================================================

//void FiffStreamThread::readProc(QTcpSocket& p_qTcpSocket)
//{
//    FiffStream t_FiffStreamIn(&p_qTcpSocket);

//    //
//    // Read: Wait 10ms for incomming tag header, read and continue
//    //
//    p_qTcpSocket.waitForReadyRead(10);

//    if (p_qTcpSocket.bytesAvailable() >= (int)sizeof(qint32)*4)
//    {
//        qDebug() << "goes to read bytes " ;
//        FiffTag::SPtr t_pTag;
//        FiffTag::read_tag_info(&t_FiffStreamIn, t_pTag, false);

//        //
//        // wait until tag size data are available and read the data
//        //
//        while (p_qTcpSocket.bytesAvailable() < t_pTag->size())
//        {
//            p_qTcpSocket.waitForReadyRead(10);
//        }
//        FiffTag::read_tag_data(&t_FiffStreamIn, t_pTag);

//        //
//        // Parse the tag
//        //
//        if(t_pTag->kind == FIFF_MNE_RT_COMMAND)
//        {
//            parseCommand(t_pTag);
//        }
//    }
//}

//=============================================================================================================

void FiffStreamThread::run()
{
    m_bIsRunning = true;

    FiffStreamServer* t_pParentServer = qobject_cast<FiffStreamServer*>(this->parent());

    connect(t_pParentServer, &FiffStreamServer::remitMeasInfo,
            this, &FiffStreamThread::sendMeasurementInfo);
    connect(t_pParentServer, &FiffStreamServer::remitRawBuffer,
            this, &FiffStreamThread::sendRawBuffer);
    connect(t_pParentServer, &FiffStreamServer::startMeasFiffStreamClient,
            this, &FiffStreamThread::startMeas);
    connect(t_pParentServer, &FiffStreamServer::stopMeasFiffStreamClient,
            this, &FiffStreamThread::stopMeas);

    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(m_iSocketDescriptor)) {
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

//    int i = 0;
    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {
        //
        // Write available data
        //
        m_qMutex.lock();
        qint32 t_iBlockSize = m_qSendBlock.size();
//        qDebug() << "data available" << t_iBlockSize;
        if(t_iBlockSize > 0)
        {
            qint32 t_iBytesWritten = t_qTcpSocket.write(m_qSendBlock);
//            qDebug() << ++i<< "[wrote bytes] " << t_iBytesWritten;
            t_qTcpSocket.waitForBytesWritten();
            if(t_iBytesWritten == t_iBlockSize)
            {
                m_qSendBlock.clear();
            }
            else
            {
                //we have to store bytes which were not written to the socket, due to writing limit
                //m_qSendBlock = m_qSendBlock.mid(t_iBytesWritten, t_iBlockSize-t_iBytesWritten);
                m_qSendBlock.remove(0,t_iBytesWritten); //higher performance then mid
            }
        }
        m_qMutex.unlock();

        //
        // Read: Wait 10ms for incomming tag header, read and continue
        //
        t_qTcpSocket.waitForReadyRead(10);

        if (t_qTcpSocket.bytesAvailable() >= (int)sizeof(qint32)*4)
        {
//            qDebug() << "goes to read bytes " ;
            FiffTag::SPtr t_pTag;
            t_FiffStreamIn.read_tag_info(t_pTag, false);

            //
            // wait until tag size data are available and read the data
            //
            while (t_qTcpSocket.bytesAvailable() < t_pTag->size())
            {
                t_qTcpSocket.waitForReadyRead(10);
            }
            t_FiffStreamIn.read_tag_data(t_pTag);

            //
            // Parse the tag
            //
            if(t_pTag->kind == FIFF_MNE_RT_COMMAND)
            {
                parseCommand(t_pTag);
            }
        }
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
