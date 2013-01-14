//=============================================================================================================
/**
* @file     mne_rt_cmd_client.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           To Be continued...
*
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
* @brief    Contains the implementation of the MNERtCmdClient Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_cmd_client.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERtCmdClient::MNERtCmdClient(QObject *parent)
: QTcpSocket(parent)
{

}


//*************************************************************************************************************

void MNERtCmdClient::connectToHost(QString &p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4217);
}


//*************************************************************************************************************

QString MNERtCmdClient::sendCommand(const QString &p_sCommand)
{
    QString t_sCommand = QString("%1\n").arg(p_sCommand);
    QString p_sReply;

    if(this->state() == QAbstractSocket::ConnectedState)
    {
        this->write(t_sCommand.toUtf8().constData(),t_sCommand.size());
        this->waitForBytesWritten();

        //thats not the most elegant way
        this->waitForReadyRead(1000);
        QByteArray t_qByteArrayRaw;
        while (this->bytesAvailable() > 0 && this->canReadLine())
            t_qByteArrayRaw += this->readAll();

        p_sReply = QString(t_qByteArrayRaw);
    }
    return p_sReply;
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeasInfo(qint32 p_id)
{
    QString t_sCommand = QString("measinfo %1").arg(p_id);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeasInfo(const QString &p_Alias)
{
    QString t_sCommand = QString("measinfo %1").arg(p_Alias);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeas(qint32 p_id)
{
    QString t_sCommand = QString("meas %1").arg(p_id);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::requestMeas(QString p_Alias)
{
    QString t_sCommand = QString("meas %1").arg(p_Alias);
    this->sendCommand(t_sCommand);
}


//*************************************************************************************************************

void MNERtCmdClient::stopAll()
{
    QString t_sCommand = QString("stop-all");
    this->sendCommand(t_sCommand);
}
