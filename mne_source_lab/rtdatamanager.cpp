//=============================================================================================================
/**
* @file     rtdatamanager.cpp
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
* @brief    Contains the implementation of the RtClient Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdatamanager.h"
#include "sourcelab.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../MNE/mne/mne_rt_cmd_client.h"
#include "../MNE/mne/mne_rt_data_client.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtNetwork>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtDataManager::RtDataManager(SourceLab* p_pSourceLab, QObject *parent)
: QThread(parent)
, m_pSourceLab(p_pSourceLab)
, m_sRtServerHostName("127.0.0.1")
{
}


//*************************************************************************************************************

RtDataManager::~RtDataManager()
{
    stop();
}


//*************************************************************************************************************

bool RtDataManager::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void RtDataManager::run()
{
    m_bIsRunning = true;

    qDebug() << "void RtDataManager::run()";

    MNERtCmdClient t_cmdClient;
    t_cmdClient.connectToHost(m_sRtServerHostName);
    t_cmdClient.waitForConnected();

    MNERtDataClient t_dataClient;
    t_dataClient.connectToHost(m_sRtServerHostName);
    t_dataClient.waitForConnected();

    // set data client alias -> for convinience (optional)
    t_dataClient.setClientAlias("mne_source_lab"); // used in option 2 later on

    // example commands
    qDebug() << t_cmdClient.sendCommand("help");
    qDebug() << t_cmdClient.sendCommand("clist");
    qDebug() << t_cmdClient.sendCommand("conlist");

    qint32 clientId = t_dataClient.getClientId();

    // read meas info
    t_cmdClient.requestMeasInfo(clientId);

    m_pSourceLab->mutex.lock();
    if(m_pSourceLab->m_pRawInfo)
        delete m_pSourceLab->m_pRawInfo;
    m_pSourceLab->m_pRawInfo = t_dataClient.readInfo();
    m_pSourceLab->mutex.unlock();

    // start measurement
    t_cmdClient.requestMeas(clientId);

    MatrixXf t_matRawBuffer;

    fiff_int_t kind;

    qint32 from = 0;
    qint32 to = -1;

    qint32 sampleSize = 0;

    while(m_bIsRunning)
    {
        t_dataClient.readRawBuffer(m_pSourceLab->m_pRawInfo->nchan, t_matRawBuffer, kind);


        if(kind == FIFF_DATA_BUFFER)
        {
            //When sample buffer size is changed reininit circular buffer
            if(sampleSize != t_matRawBuffer.cols())
            {
                if(m_pSourceLab->isRunning())
                    m_pSourceLab->stop();
                if(m_pSourceLab->m_pRawMatrixBuffer)
                    delete m_pSourceLab->m_pRawMatrixBuffer;
                m_pSourceLab->m_pRawMatrixBuffer = new RawMatrixBuffer(10, m_pSourceLab->m_pRawInfo->nchan, t_matRawBuffer.cols());

                sampleSize = t_matRawBuffer.cols();
                m_pSourceLab->start();
            }

            to += t_matRawBuffer.cols();
            printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/m_pSourceLab->m_pRawInfo->sfreq, ((float)to)/m_pSourceLab->m_pRawInfo->sfreq);
            from += t_matRawBuffer.cols();

            m_pSourceLab->m_pRawMatrixBuffer->push(&t_matRawBuffer);
        }
        else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
            m_bIsRunning;

        printf("[done]\n");
    }
}
