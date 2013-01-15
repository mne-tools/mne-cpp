//=============================================================================================================
/**
* @file     BabyMEG.cpp
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
* @brief     implementation of the BabyMEG Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"
#include "dacqserver.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../../../MNE/fiff/fiff.h"
#include "../../../MNE/fiff/fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../../../MNE/mne/mne.h"
#include "../../../MNE/mne/mne_epoch_data_list.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_pDacqServer(new DacqServer(this))
, m_pInfo(new FiffInfo())
, m_bIsRunning(false)
, m_iID(-1)
{
    this->setBufferSampleSize(100);
    m_pRawMatrixBuffer = NULL;
    this->init();
//    this->start();
}


//*************************************************************************************************************

BabyMEG::~BabyMEG()
{
    qDebug() << "Destroy BabyMEG::~BabyMEG()";

    delete m_pDacqServer;

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

QByteArray BabyMEG::availableCommands()
{
    QByteArray t_blockCmdInfoList;

//    t_blockCmdInfoList.append(QString("\t### %1 connector###\r\n").arg(this->getName()));

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

ConnectorID BabyMEG::getConnectorID() const
{
    return _BABYMEG;
}


//*************************************************************************************************************

const char* BabyMEG::getName() const
{
    return "BabyMEG Connector";
}


//*************************************************************************************************************

void BabyMEG::init()
{

}


//*************************************************************************************************************

bool BabyMEG::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    bool success = false;

    return success;
}


//*************************************************************************************************************

bool BabyMEG::start()
{
    // Start thread
    m_pDacqServer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::stop()
{
    m_bIsRunning = false;
    QThread::wait();//ToDo: This thread will never be terminated when circular buffer is blocking the thread (happens when circularbuffer is empty)
    
    m_pDacqServer->m_bIsRunning = false;
    m_pDacqServer->wait();

    qDebug() << "bool BabyMEG::stop()";

    return true;
}


//*************************************************************************************************************

void BabyMEG::releaseMeasInfo()
{
    if(!m_pInfo->isEmpty())
        emit remitMeasInfo(m_iID, m_pInfo);
}


//*************************************************************************************************************

void BabyMEG::requestMeasInfo(qint32 ID)
{
    m_iID = ID;

    if(!m_pInfo->isEmpty())
        releaseMeasInfo();
    else
    {
        m_pDacqServer->m_bMeasInfoRequest = true;

        //This should never happen
        if(m_pDacqServer->isRunning())
        {
            m_pDacqServer->m_bIsRunning = false;
            m_pDacqServer->wait();
            m_pDacqServer->start();
        }
        //
        else
        {
            m_pDacqServer->start();
//            m_pDacqServer->wait();// until header reading finished
        }
    }
}


//*************************************************************************************************************

void BabyMEG::requestMeas()
{
    qDebug() << "void BabyMEG::requestMeas()";

    m_pDacqServer->m_bMeasRequest = true;
    this->start();
}


//*************************************************************************************************************

void BabyMEG::requestMeasStop()
{
    this->stop();
}


//*************************************************************************************************************

void BabyMEG::requestSetBufferSize(quint32 p_uiBuffSize)
{
    if(p_uiBuffSize > 0)
    {
        qDebug() << "void BabyMEG::setBufferSize: " << p_uiBuffSize;

        this->stop();

        this->setBufferSampleSize(p_uiBuffSize);

        this->start();
    }
}


//*************************************************************************************************************

void BabyMEG::run()
{
    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            // Pop available Buffers
            MatrixXf tmp = m_pRawMatrixBuffer->pop();
//            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

            emit remitRawBuffer(tmp);
        }
    }
}
