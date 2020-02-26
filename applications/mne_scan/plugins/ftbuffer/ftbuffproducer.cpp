/**
* @file     ftbuffproducer.cpp
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the FtBuffProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffproducer.h"
#include "ftbuffer.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//*************************************************************************************************************

FtBuffProducer::FtBuffProducer(FtBuffer* pFtBuffer)
: m_pFtBuffer(pFtBuffer)
{
    m_pFtConnector = new FtConnector();
}

//*************************************************************************************************************

FtBuffProducer::~FtBuffProducer()
{
    delete m_pFtConnector;
}

//*************************************************************************************************************

void FtBuffProducer::run()
{
    qInfo() << "[FtBuffProducer::run] Running producer..";

    while(!m_pFtConnector->connect()) {
        QThread::usleep(50000);
    }

    while(!m_pFtConnector->getHeader()) {
        QThread::usleep(50000);
    }

    while (true) {
        m_pFtConnector->getData();

        if (m_pFtConnector->newData()) {
            qInfo() << "[FtBuffProducer::run] Returning new data matrix.";
            emit newDataAvailable(m_pFtConnector->getMatrix());
            m_pFtConnector->resetEmitData();
        }

        QThread::usleep(50);

        m_pFtBuffer->m_mutex.lock();
        if (!m_pFtBuffer->isRunning()) {
            break;
        }
        m_pFtBuffer->m_mutex.unlock();
    }
}

//*************************************************************************************************************

void FtBuffProducer::doWork()
{
    run();
}

//*************************************************************************************************************

void FtBuffProducer::connectToBuffer(QString addr,
                                     int port)
{
//    m_pTempAddress = new char[(addr.toLocal8Bit().size()) + 1];
//    strcpy(m_pTempAddress, addr.toLocal8Bit().constData());

//    if (m_pFtConnector != Q_NULLPTR)
//        delete m_pFtConnector;
    if (m_pFtConnector != Q_NULLPTR) {
        delete m_pFtConnector;
    }

    m_pFtConnector = new FtConnector();
    m_pFtConnector->setAddr(addr);
    m_pFtConnector->setPort(port);

    if(!m_pFtBuffer->setupRTMSA()){
        qInfo() << "[FtBuffProducer::connectToBuffer] Attempting to read neuromag header from buffer...";
        if(!m_pFtConnector->connect()) {
            emit connecStatus(false);
            return;
        }
        if (!m_pFtBuffer->setupRTMSA(m_pFtConnector->parseNeuromagHeader())) {
            qInfo() << "[FtBuffProducer::connectToBuffer] Failed to read neuromag header from buffer.";
            emit connecStatus(false);
            return;
        }
    }

    emit connecStatus(true);
}

//*************************************************************************************************************

bool FtBuffProducer::disconnectFromBuffer()
{
    return this->m_pFtBuffer->disconnect();
}

//*************************************************************************************************************

