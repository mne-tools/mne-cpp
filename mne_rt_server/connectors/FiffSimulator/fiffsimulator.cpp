//=============================================================================================================
/**
* @file     fiffsimulator.cpp
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
* @brief    Contains the implementation of the FiffSimulator Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"
#include "fiffproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <mne/mne_epoch_data_list.h>


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

using namespace FiffSimulatorPlugin;
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_pFiffProducer(new FiffProducer(this))
, m_pRawInfo(NULL)
, m_sResourceDataPath("../../mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
, m_bIsRunning(false)
{
    this->setBufferSampleSize(1000);
    m_pRawMatrixBuffer = NULL;
    this->init();
}


//*************************************************************************************************************

FiffSimulator::~FiffSimulator()
{
    qDebug() << "Destroy FiffSimulator::~FiffSimulator()";

    delete m_pFiffProducer;

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

QByteArray FiffSimulator::availableCommands()
{
    QByteArray t_blockCmdInfoList;

//    t_blockCmdInfoList.append(QString("\t### %1 connector###\r\n").arg(this->getName()));

    t_blockCmdInfoList.append(QString("\tsimfile  [file]\t\t%1: the fiff file which should be used as simulation file.\r\n").arg(this->getName()));
    t_blockCmdInfoList.append(QString("\tbufsize  [samples]\t%1: sets the buffer size of the FiffStreamClient\r\n\t\t\t\traw data buffers\r\n").arg(this->getName()));

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

ConnectorID FiffSimulator::getConnectorID() const
{
    return _FIFFSIMULATOR;
}


//*************************************************************************************************************

const char* FiffSimulator::getName() const
{
    return "Fiff File Simulator";
}


//*************************************************************************************************************

void FiffSimulator::init()
{
    if(m_pRawMatrixBuffer)
        delete m_pRawMatrixBuffer;
    m_pRawMatrixBuffer = NULL;

    if(m_pRawInfo)
        m_pRawMatrixBuffer = new RawMatrixBuffer(RAW_BUFFFER_SIZE, m_pRawInfo->info.nchan, this->getBufferSampleSize());
}


//*************************************************************************************************************

bool FiffSimulator::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
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
    } else if(p_sListCommand[0].compare("simfile",Qt::CaseInsensitive) == 0)
    {
        //
        // simulation file
        //
        printf("simfile\r\n");

        QFile t_file(p_sListCommand[1]);

        QString t_sResourceDataPathOld = m_sResourceDataPath;

        if(t_file.exists())
        {
            m_sResourceDataPath = p_sListCommand[1];
            m_pRawInfo = false;

            if (this->readRawInfo())
            {
                m_pFiffProducer->stop();
                this->stop();

                p_blockOutputInfo.append("New simulation file set succefully.\r\n");
            }
            else
            {
                qDebug() << "Don't set new file";
                m_sResourceDataPath = t_sResourceDataPathOld;

                p_blockOutputInfo.append("Simulation file not set.\r\n");
            }
        }
        success = true;
    }

    return success;
}


//*************************************************************************************************************

bool FiffSimulator::start()
{
    this->init();

    // Start threads
    m_pFiffProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool FiffSimulator::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void FiffSimulator::requestMeasInfo(qint32 ID)
{

    if(!m_pRawInfo)
        readRawInfo();

    if(m_pRawInfo)
        emit remitMeasInfo(ID, m_pRawInfo->info);
//    else
//        return NULL;
}


//*************************************************************************************************************

void FiffSimulator::requestMeas()
{
    this->m_pFiffProducer->start();
    this->start();
}


//*************************************************************************************************************

void FiffSimulator::requestMeasStop()
{
    this->m_pFiffProducer->stop();
    this->stop();
}


//*************************************************************************************************************

void FiffSimulator::requestSetBufferSize(quint32 p_uiBuffSize)
{
    if(p_uiBuffSize > 0)
    {
//        qDebug() << "void FiffSimulator::requestSetBufferSize: " << p_uiBuffSize;

        m_pFiffProducer->stop();
        this->stop();

        this->setBufferSampleSize(p_uiBuffSize);

        this->start();

    }
}


//*************************************************************************************************************

bool FiffSimulator::readRawInfo()
{
    if(!m_pRawInfo)
    {
        QFile* t_pFile = new QFile(m_sResourceDataPath);

        mutex.lock();
        if(!FiffStream::setup_read_raw(t_pFile, m_pRawInfo))
        {
            printf("Error: Not able to read raw info!\n");
            if(m_pRawInfo)
                delete m_pRawInfo;
            m_pRawInfo = NULL;

            delete t_pFile;
            return false;
        }

        //delete it here and reopen it in the producer thread
        delete m_pRawInfo->file;
        m_pRawInfo->file = NULL;

        if(m_pRawMatrixBuffer)
            delete m_pRawMatrixBuffer;
        m_pRawMatrixBuffer = new RawMatrixBuffer(10, m_pRawInfo->info.nchan, getBufferSampleSize());

        mutex.unlock();

        delete t_pFile;
    }

    return true;
}


//*************************************************************************************************************

void FiffSimulator::run()
{
    m_bIsRunning = true;

    float t_fSamplingFrequency = m_pRawInfo->info.sfreq;
    float t_fBuffSampleSize = (float)getBufferSampleSize();

    quint32 uiSamplePeriod = (unsigned int) ((t_fBuffSampleSize/t_fSamplingFrequency)*1000000.0f);
    quint32 count = 0;

    while(m_bIsRunning)
    {
        MatrixXf tmp = m_pRawMatrixBuffer->pop();
        ++count;
//        printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

        emit remitRawBuffer(tmp);
        usleep(uiSamplePeriod);
    }
}
