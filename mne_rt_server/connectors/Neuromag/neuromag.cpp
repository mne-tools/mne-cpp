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
: m_pRawInfo(NULL)
, m_bIsRunning(false)
{
    this->setBufferSampleSize(1000);
    m_pRawMatrixBuffer = NULL;
    this->init();
}


//*************************************************************************************************************

Neuromag::~Neuromag()
{
    qDebug() << "Destroy FiffSimulator::~FiffSimulator()";

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

QByteArray Neuromag::availableCommands() const
{
    QByteArray t_blockCmdInfoList;

    t_blockCmdInfoList.append(QString("\t### %1 connector###\r\n").arg(this->getName()));

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
    if(m_pRawMatrixBuffer)
        delete m_pRawMatrixBuffer;
    m_pRawMatrixBuffer = NULL;

    if(m_pRawInfo)
        m_pRawMatrixBuffer = new RawMatrixBuffer(10, m_pRawInfo->info->nchan, this->getBufferSampleSize());
}


//*************************************************************************************************************

bool Neuromag::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    bool success = false;
//    if(p_sListCommand[0].compare("simfile",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // simulation file
//        //
//        printf("simfile\r\n");

//        QFile t_file(p_sListCommand[1]);

//        QString t_sResourceDataPathOld = m_sResourceDataPath;

//        if(t_file.exists())
//        {
//            m_sResourceDataPath = p_sListCommand[1];
//            m_pRawInfo = false;

//            if (this->readRawInfo())
//            {
//                m_pFiffProducer->stop();
//                this->stop();

//                p_blockOutputInfo.append("New simulation file set succefully.\r\n");
//            }
//            else
//            {
//                qDebug() << "Don't set new file";
//                m_sResourceDataPath = t_sResourceDataPathOld;

//                p_blockOutputInfo.append("Simulation file not set.\r\n");
//            }
//        }
//        success = true;
//    }

    return success;
}


//*************************************************************************************************************

bool Neuromag::start()
{
    this->init();

    // Start thread
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Neuromag::stop()
{
    m_bIsRunning = false;

    QThread::wait();

    return true;
}


//*************************************************************************************************************

void Neuromag::requestMeasInfo(qint32 ID)
{

    if(!m_pRawInfo)
        readRawInfo();

    if(m_pRawInfo)
        emit remitMeasInfo(ID, m_pRawInfo->info);
//    else
//        return NULL;
}


//*************************************************************************************************************

void Neuromag::requestMeas()
{
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
        qDebug() << "void FiffSimulator::requestSetBufferSize: " << p_uiBuffSize;

        this->stop();

        this->setBufferSampleSize(p_uiBuffSize);

        this->start();
    }
}


//*************************************************************************************************************

bool Neuromag::readRawInfo()
{
//    if(!m_pRawInfo)
//    {
//        QFile* t_pFile = new QFile(m_sResourceDataPath);

//        mutex.lock();
//        if(!FiffStream::setup_read_raw(t_pFile, m_pRawInfo))
//        {
//            printf("Error: Not able to read raw info!\n");
//            if(m_pRawInfo)
//                delete m_pRawInfo;
//            m_pRawInfo = NULL;

//            delete t_pFile;
//            return false;
//        }

//        //delete it here and reopen it in the producer thread
//        delete m_pRawInfo->file;
//        m_pRawInfo->file = NULL;

//        if(m_pRawMatrixBuffer)
//            delete m_pRawMatrixBuffer;
//        m_pRawMatrixBuffer = new RawMatrixBuffer(10, m_pRawInfo->info->nchan, getBufferSampleSize());

//        mutex.unlock();

//        delete t_pFile;
//    }

    return true;
}


//*************************************************************************************************************

void Neuromag::run()
{
    m_bIsRunning = true;

//    float t_fSamplingFrequency = m_pRawInfo->info->sfreq;
//    float t_fBuffSampleSize = (float)getBufferSampleSize();

//    quint32 uiSamplePeriod = (unsigned int) ((t_fBuffSampleSize/t_fSamplingFrequency)*1000000.0f);
    quint32 count = 0;

    while(m_bIsRunning)
    {
//        MatrixXf tmp = m_pRawMatrixBuffer->pop();
        ++count;
////        printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

//        emit remitRawBuffer(tmp);
//        usleep(uiSamplePeriod);
    }
}
