//=============================================================================================================
/**
 * @file     ftbuffproducer.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel B Motta, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffproducer.h"
#include "ftbuffer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBuffProducer::FtBuffProducer(FtBuffer* pFtBuffer)
: m_pFtBuffer(pFtBuffer)
{
    m_pFtConnector = new FtConnector();
}

//=============================================================================================================

FtBuffProducer::~FtBuffProducer()
{
    delete m_pFtConnector;
}

//=============================================================================================================

void FtBuffProducer::runMainLoop()
{
    qInfo() << "[FtBuffProducer::runMainLoop] Running producer...";

    while(!m_pFtConnector->connect()) {
        QThread::usleep(50000);
    }

    while(!m_pFtConnector->getHeader()) {
        QThread::usleep(50000);
    }

    m_pFtConnector->catchUpToBuffer();

    qInfo() << "[FtBuffProducer::runMainLoop] Connected to buffer and ready to receive data.";

    while(!this->thread()->isInterruptionRequested()) {
        m_pFtConnector->getData();

        //Sends up new data when FtConnector flags new data
        if (m_pFtConnector->newData()) {
            emit newDataAvailable(m_pFtConnector->getMatrix());
            m_pFtConnector->resetEmitData();
        }

        //QThread::usleep(50);
    }

    this->thread()->quit();
}

//=============================================================================================================

void FtBuffProducer::doWork()
{
    runMainLoop();
}

//=============================================================================================================

void FtBuffProducer::connectToBuffer(QString addr,
                                     int port)
{
    if (m_pFtConnector != Q_NULLPTR) {
        delete m_pFtConnector;
    }

    //new parameters, new connector object
    m_pFtConnector = new FtConnector();
    m_pFtConnector->setAddr(addr);
    m_pFtConnector->setPort(port);

    //Try to get info from buffer first, then resort to file
    if(m_pFtConnector->connect()) {
        auto metadata = m_pFtConnector->parseBufferHeaders();
        if (m_pFtBuffer->setupRTMSA(metadata)){
            emit connecStatus(true);
            return;
        }
    }
    emit connecStatus(false);
}

//=============================================================================================================

bool FtBuffProducer::disconnectFromBuffer()
{
    return this->m_pFtBuffer->disconnect();
}
