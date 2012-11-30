//=============================================================================================================
/**
* @file     neuromag.cpp
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
* @brief    Contains the implementation of the Neuromag Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag.h"
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

using namespace NeuromagPlugin;
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neuromag::Neuromag()
: m_pDacqServer(new DacqServer(this))
, m_pInfo(NULL)
, m_bIsRunning(false)
, m_iID(-1)
{
    this->setBufferSampleSize(100);
    m_pRawMatrixBuffer = NULL;
    this->init();
//    this->start();
}


//*************************************************************************************************************

Neuromag::~Neuromag()
{
    qDebug() << "Destroy Neuromag::~Neuromag()";

    delete m_pDacqServer;

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

QByteArray Neuromag::availableCommands() const
{
    QByteArray t_blockCmdInfoList;

    t_blockCmdInfoList.append(QString("\t### %1 connector###\r\n").arg(this->getName()));
    t_blockCmdInfoList.append("\tbufsize  [samples]\tsets the buffer size of the FiffStreamClient\r\n\t\t\t\traw data buffers\r\n");

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

ConnectorID Neuromag::getConnectorID() const
{
    return _NEUROMAG;
}


//*************************************************************************************************************

const char* Neuromag::getName() const
{
    return "Neuromag Connector";
}


//*************************************************************************************************************

void Neuromag::init()
{

}


//*************************************************************************************************************

bool Neuromag::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    bool success = false;


    if(p_sListCommand[0].compare("bufsize",Qt::CaseInsensitive) == 0)
    {
        //
        // bufsize
        //
        if(p_sListCommand.size() > 1)
        {
            bool ok;
            quint32 t_uiBuffSize = p_sListCommand[1].toInt(&ok);

            if(ok && t_uiBuffSize > 0)
            {
                printf("bufsize %d\n", t_uiBuffSize);

                requestSetBufferSize(t_uiBuffSize);

                QString str = QString("\tSet %1 buffer sample size to %2 samples\r\n\n").arg(getName()).arg(t_uiBuffSize);
                p_blockOutputInfo.append(str);
            }
            else
            {
                p_blockOutputInfo.append("\tBuffer size not set\r\n\n");
            }
        }
        success = true;
    }

    return success;
}


//*************************************************************************************************************

bool Neuromag::start()
{
    // Start thread
    m_pDacqServer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Neuromag::stop()
{
    m_pDacqServer->m_bIsRunning = false;
    m_pDacqServer->wait();

    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void Neuromag::releaseMeasInfo()
{
    if(m_pInfo)
        emit remitMeasInfo(m_iID, m_pInfo);
}


//*************************************************************************************************************

void Neuromag::requestMeasInfo(qint32 ID)
{
    m_iID = ID;

    if(m_pInfo)
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

void Neuromag::requestMeas()
{
    qDebug() << "void Neuromag::requestMeas()";

    m_pDacqServer->m_bMeasRequest = true;
    this->start();
}


//*************************************************************************************************************

void Neuromag::requestMeasStop()
{
    this->stop();
}


//*************************************************************************************************************

void Neuromag::requestSetBufferSize(quint32 p_uiBuffSize)
{
    if(p_uiBuffSize > 0)
    {
        qDebug() << "void Neuromag::setBufferSize: " << p_uiBuffSize;

        this->stop();

        this->setBufferSampleSize(p_uiBuffSize);

        this->start();
    }
}


//*************************************************************************************************************

void Neuromag::run()
{
    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            // Pop available Buffers
            MatrixXf tmp = m_pRawMatrixBuffer->pop();

////            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

            emit remitRawBuffer(tmp);
        }
    }
}
