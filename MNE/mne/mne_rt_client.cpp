//=============================================================================================================
/**
* @file     mne_rt_client.cpp
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
* @brief     implementation of the MNERtClient Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_client.h"
#include "mne_rt_cmd_client.h"
#include "mne_rt_data_client.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERtClient::MNERtClient(QString p_sRtServerHostname, QObject *parent)
: QThread(parent)
, m_bIsRunning(false)
, m_sRtServerHostName(p_sRtServerHostname)
, m_pFiffInfo(new FiffInfo)
{
}


//*************************************************************************************************************

MNERtClient::~MNERtClient()
{
    stop();
}


//*************************************************************************************************************

bool MNERtClient::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void MNERtClient::run()
{
    m_bIsRunning = true;

    MNERtCmdClient t_cmdClient;
    t_cmdClient.connectToHost(m_sRtServerHostName);
    t_cmdClient.waitForConnected();

    MNERtDataClient t_dataClient;
    t_dataClient.connectToHost(m_sRtServerHostName);
    t_dataClient.waitForConnected();

    // set data client alias -> for convinience (optional)
    t_dataClient.setClientAlias("mne_source_lab"); // used in option 2 later on

//    // example commands
//    qDebug() << t_cmdClient.sendCommand("help");
//    qDebug() << t_cmdClient.sendCommand("clist");
//    qDebug() << t_cmdClient.sendCommand("conlist");

    qint32 clientId = t_dataClient.getClientId();

    // read meas info
    t_cmdClient.requestMeasInfo(clientId);

    m_pFiffInfo = new FiffInfo(t_dataClient.readInfo());

    // start measurement
    t_cmdClient.requestMeas(clientId);

    MatrixXf t_matRawBuffer;

    fiff_int_t kind;

    qint32 from = 0;
    qint32 to = -1;

    while(m_bIsRunning)
    {
        t_dataClient.readRawBuffer(m_pFiffInfo->nchan, t_matRawBuffer, kind);

        if(kind == FIFF_DATA_BUFFER)
        {
            to += t_matRawBuffer.cols();
            printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/m_pFiffInfo->sfreq, ((float)to)/m_pFiffInfo->sfreq);
            from += t_matRawBuffer.cols();

            emit rawBufferReceived(t_matRawBuffer);
        }
        else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
            m_bIsRunning = false;

        printf("[done]\n");
    }
}
