//=============================================================================================================
/**
 * @file     neuromag.cpp
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
 * @brief     Definition of the Neuromag Class.
 *
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag.h"

#include "dacqserver.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGRTSERVERPLUGIN;
using namespace FIFFLIB;
using namespace RTSERVER;
using namespace COMMUNICATIONLIB;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neuromag::Neuromag()
: m_pDacqServer(QSharedPointer<DacqServer>(new DacqServer(this)))
, m_iID(-1)
, m_uiBufferSampleSize(600)
, m_bIsRunning(false)
{
    this->init();
}


//=============================================================================================================

Neuromag::~Neuromag()
{
    qDebug() << "Destroy Neuromag::~Neuromag()";

    m_bIsRunning = false;
    QThread::wait();
}


//=============================================================================================================

void Neuromag::comBufsize(Command p_command)
{
    quint32 t_uiBuffSize = p_command.pValues()[0].toUInt();

    if(t_uiBuffSize > 0)
    {
        qDebug() << "void Neuromag::setBufferSize: " << t_uiBuffSize;

        bool t_bWasRunning = m_bIsRunning;

        if(m_bIsRunning)
            this->stop();

        m_uiBufferSampleSize = t_uiBuffSize;

        if(t_bWasRunning)
            this->start();

        QString str = QString("\tSet %1 buffer sample size to %2 samples\r\n\n").arg(getName()).arg(t_uiBuffSize);

        m_commandManager["bufsize"].reply(str);
    }
    else
        m_commandManager["bufsize"].reply("Buffer size not set\r\n");
}


//=============================================================================================================

void Neuromag::comGetBufsize(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if(t_bCommandIsJson)
    {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert("bufsize", QJsonValue((double)m_uiBufferSampleSize));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager["getbufsize"].reply(p_qJsonDocument.toJson());
    }
    else
    {
        QString str = QString("\t%1\r\n\n").arg(m_uiBufferSampleSize);
        m_commandManager["getbufsize"].reply(str);
    }
}


//=============================================================================================================

void Neuromag::connectCommandManager()
{
    //Connect slots
    QObject::connect(&m_commandManager["bufsize"], &Command::executed, this, &Neuromag::comBufsize);
    QObject::connect(&m_commandManager["getbufsize"], &Command::executed, this, &Neuromag::comGetBufsize);

}


//=============================================================================================================

ConnectorID Neuromag::getConnectorID() const
{
    return _NEUROMAG;
}


//=============================================================================================================

const char* Neuromag::getName() const
{
    return "Neuromag Connector";
}


//=============================================================================================================

void Neuromag::init()
{

}


//=============================================================================================================

void Neuromag::info(qint32 ID)
{
    m_iID = ID;

    if(!m_info.isEmpty())
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


//=============================================================================================================

void Neuromag::releaseMeasInfo()
{
    if(!m_info.isEmpty())
        emit remitMeasInfo(m_iID, m_info);
}

//=============================================================================================================

bool Neuromag::start()
{
    qDebug() << "bool Neuromag::start()";

    m_pDacqServer->m_bMeasRequest = true;

    // Start thread
    m_pDacqServer->start();

    QThread::start();

    return true;
}


//=============================================================================================================

bool Neuromag::stop()
{
    m_bIsRunning = false;
    QThread::wait();//ToDo: This thread will never be terminated when circular buffer is blocking the thread (happens when circularbuffer is empty)
    
    m_pDacqServer->m_bIsRunning = false;
    m_pDacqServer->wait();

    qDebug() << "bool Neuromag::stop()";

    return true;
}


//=============================================================================================================

void Neuromag::run()
{
    m_bIsRunning = true;

    //qint32 count = 0;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            // Pop available Buffers
            QSharedPointer<Eigen::MatrixXf> t_pRawBuffer(new Eigen::MatrixXf(m_pRawMatrixBuffer->pop()));
//            ++count;
//            printf("%d raw buffer (%d x %d) generated\r\n", count, t_pRawBuffer->rows(), t_pRawBuffer->cols());

            emit remitRawBuffer(t_pRawBuffer);
        }
    }
}
