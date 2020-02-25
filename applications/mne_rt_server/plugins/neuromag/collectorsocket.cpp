//=============================================================================================================
/**
 * @file     collectorsocket.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
 * @brief     Definition of the CollectorSocket class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "collectorsocket.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGRTSERVERPLUGIN;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CollectorSocket::CollectorSocket(QObject *parent)
: QTcpSocket(parent)
, m_sCollectorHost(QHostAddress(QHostAddress::LocalHost).toString())
, m_bIsMeasuring(false)
{

}


//=============================================================================================================

bool CollectorSocket::open()
{
    printf("About to connect to collector... ");
    if(this->state() == QAbstractSocket::ConnectedState)
    {
        printf("Note: Tried to re-open an open connection... [done]\r\n");//dacq_log("Note: Tried to re-open an open connection\n");
        return true;
    }
    this->abort();
    this->connectToHost(m_sCollectorHost, COLLECTOR_PORT);
    this->waitForConnected( -1 );//qDebug() << "Socket Stat: " << this->state();
    printf("[done]\r\n");

    if (!this->server_login(QString(COLLECTOR_PASS), QString("mne_rt_server"))) {
        printf("Neuromag collector connection: Error\r\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return false;
    }

    return true;
}


//=============================================================================================================

//int CollectorSocket::close()
//{

//    qDebug() << "DacqServer::collector_close()";

//    if (this == NULL)
//        return(0);
////    if (dacq_server_close(&this, NULL)) {
////        printf("Neuromag collector connection: Eror\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
////        return(-1);
////    }

//    this->waitForReadyRead(100);

//    this->close();

////    m_iCollectorSock = -1;
//    return(0);
//}


//=============================================================================================================

int CollectorSocket::getMaxBuflen()
{
// ToDo doesn't work without setting first buffersize first
    int maxbuflen = -1;

    QString t_sSend = QString("%1\r\n").arg(COLLECTOR_GETVARS);

    QByteArray t_buf;
    if (!this->server_send(t_sSend, t_buf, DACQ_KEEP_INPUT)) {
        printf("Neuromag collector connection: Error\r\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return -1;
    }

    QList<QByteArray> t_lLinesBuffer = t_buf.split('\n');

    QByteArray bufVar1(COLLECTOR_BUFVAR);
    QByteArray bufVar2("Vectors in a buffer");
    for(qint32 i = 0; i < t_lLinesBuffer.size(); ++i)
    {
        if(t_lLinesBuffer[i].contains(bufVar1)) //option 1
        {
            char var_name[1024];
            char var_value[1024];
            char var_type[1024];
            sscanf(t_lLinesBuffer[i].data()+4, "%s %s %s", var_name, var_value, var_type);
            maxbuflen = QString(var_value).toInt();
            return maxbuflen;
        }
        else if(t_lLinesBuffer[i].contains(bufVar2)) //option 2
        {
            QList<QByteArray> t_lMaxBuflen = t_lLinesBuffer[i].split(':');
            maxbuflen = t_lMaxBuflen[t_lMaxBuflen.size()-1].trimmed().toInt();
            return maxbuflen;
        }
    }

    return -1;
}


//=============================================================================================================

int CollectorSocket::setMaxBuflen(int maxbuflen)
{
    if (maxbuflen < 1)
        return(0);

    if (!this->server_command(QString("%1 %2 %3\n").arg(COLLECTOR_SETVARS).arg(COLLECTOR_BUFVAR).arg(maxbuflen)) ||
            !this->server_command(QString("%1\n").arg(COLLECTOR_DOSETUP)))
    {
        printf("Neuromag collector connection: Error\n");//dacq_log("Neuromag collector connection: %s\n", err_get_error());
        return(-1);
    }

    return(0);
}


//=============================================================================================================

bool CollectorSocket::server_command(const QString& p_sCommand)
{

    if (this->state() != QAbstractSocket::ConnectedState)
        return false;

    //ToDo Command Check
    QByteArray t_arrCommand = p_sCommand.toLocal8Bit();

    if(t_arrCommand.size() > 0)
    {
        this->write(t_arrCommand);
        this->flush();
    }

    //ToDo check if command was succefull processed by the collector
    this->waitForReadyRead(-1);
    this->readAll();//readAll that QTcpSocket is empty again

    return true;
}


//=============================================================================================================

bool CollectorSocket::server_login(const QString& p_sCollectorPass, const QString& p_sMyName)
{
    printf("Login... ");

    QString t_sCommand = QString("%1 %2\r\n").arg(DACQ_CMD_PASSWORD).arg(p_sCollectorPass);
    this->server_command(t_sCommand);

    t_sCommand = QString("%1 %2\r\n").arg(DACQ_CMD_NAME).arg(p_sMyName);
    this->server_command(t_sCommand);

    printf("[done]\r\n");

    return true;
}


//=============================================================================================================

bool CollectorSocket::server_send(QString& p_sDataSend, QByteArray& p_dataOut, int p_iInputFlag)
{
    if (this->state() != QAbstractSocket::ConnectedState)
        return false;

    QByteArray t_arrSend = p_sDataSend.toLocal8Bit();

    if(t_arrSend.size() > 0)
    {
        this->write(t_arrSend);
        this->flush();
    }

    this->waitForReadyRead(-1);
    if ( p_iInputFlag == DACQ_DRAIN_INPUT )
    {
        this->readAll();//readAll that QTcpSocket is empty again -> prevent overflow
    }
    else if( p_iInputFlag == DACQ_KEEP_INPUT )
    {
        p_dataOut = this->readAll();//readAll that QTcpSocket is empty again -> prevent overflow
    }

    return true;
}


//=============================================================================================================

bool CollectorSocket::server_start()
{
    if(!m_bIsMeasuring)
    {
        printf("Start measurement... ");

        QString t_sCommand = QString("%1\r\n").arg("meas");
        this->server_command(t_sCommand);

        m_bIsMeasuring = true;
    }

    return true;
}


//=============================================================================================================

bool CollectorSocket::server_stop()
{
    if(m_bIsMeasuring)
    {
        printf("Stop measurement... ");

        QString t_sCommand = QString("%1\r\n").arg("stop");
        this->server_command(t_sCommand);

        m_bIsMeasuring = false;
        printf("[done]\r\n");
    }

    return true;
}

