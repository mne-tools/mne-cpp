//=============================================================================================================
/**
 * @file     fiffsimulatorproducer.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     April, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the FiffSimulatorProducer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulatorproducer.h"
#include "fiffsimulator.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutexLocker>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORPLUGIN;
using namespace COMMUNICATIONLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulatorProducer::FiffSimulatorProducer(FiffSimulator* p_pFiffSimulator)
: m_pFiffSimulator(p_pFiffSimulator)
, m_bDataClientIsConnected(false)
, m_iDataClientId(-1)
, m_bFlagInfoRequest(false)
{
}

//=============================================================================================================

FiffSimulatorProducer::~FiffSimulatorProducer()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

void FiffSimulatorProducer::connectDataClient(QString p_sRtSeverIP)
{
    if(m_pRtDataClient.isNull()) {
        m_pRtDataClient = QSharedPointer<RtDataClient>(new RtDataClient);
    } else if(m_bDataClientIsConnected) {
        return;
    }

    m_pRtDataClient->connectToHost(p_sRtSeverIP);
    m_pRtDataClient->waitForConnected(1000);

    if(m_pRtDataClient->state() == QTcpSocket::ConnectedState) {
        producerMutex.lock();
        if(!m_bDataClientIsConnected) {
            // get client ID
            m_iDataClientId = m_pRtDataClient->getClientId();

            // set data client alias -> for convinience (optional)
            m_pRtDataClient->setClientAlias(m_pFiffSimulator->m_sFiffSimulatorClientAlias); // used in option 2 later on

            // set new state
            m_bDataClientIsConnected = true;
            emit dataConnectionChanged(m_bDataClientIsConnected);
        }
        producerMutex.unlock();
    }
}

//=============================================================================================================

void FiffSimulatorProducer::disconnectDataClient()
{
    if(m_bDataClientIsConnected) {
        m_pRtDataClient->disconnectFromHost();
        if (m_pRtDataClient->state() != QAbstractSocket::UnconnectedState) {
            m_pRtDataClient->waitForDisconnected();
        }

        producerMutex.lock();
        m_iDataClientId = -1;
        m_bDataClientIsConnected = false;
        producerMutex.unlock();
        emit dataConnectionChanged(m_bDataClientIsConnected);
    }
}

//=============================================================================================================

void FiffSimulatorProducer::stop()
{
    requestInterruption();
    wait();
}

//=============================================================================================================

void FiffSimulatorProducer::run()
{
    // Connect data client in the same thread we receive data from it
    connectDataClient(m_pFiffSimulator->m_sFiffSimulatorIP);

    qint32 count = 0;
    while(!isInterruptionRequested() && m_pRtDataClient->state() != QTcpSocket::ConnectedState) {
        msleep(100);
        this->connectDataClient(m_pFiffSimulator->m_sFiffSimulatorIP);
        ++count;
        if(count > 10) {
            return;
        }
    }

    // Inits
    MatrixXf matData;
    fiff_int_t kind;
    qint32 from = 0;
    qint32 to = -1;

    while(!isInterruptionRequested()) {
        producerMutex.lock();
        if(m_bFlagInfoRequest) {
            m_pFiffSimulator->m_qMutex.lock();
            m_pFiffSimulator->m_pFiffInfo = m_pRtDataClient->readInfo();
            m_pFiffSimulator->m_qMutex.unlock();
            emit m_pFiffSimulator->fiffInfoAvailable();
            m_bFlagInfoRequest = false;
        }
        producerMutex.unlock();

        // Only perform data reading if the measurement was started
        if(m_pFiffSimulator->isRunning()) {
            m_pRtDataClient->readRawBuffer(m_pFiffSimulator->m_pFiffInfo->nchan,
                                           matData,
                                           kind);

            if(kind == FIFF_DATA_BUFFER) {
                to += matData.cols();
                from += matData.cols();
                while(!m_pFiffSimulator->m_pCircularBuffer->push(matData) && !isInterruptionRequested()) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            } else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END) {
                break;
            }
        }
    }

    // Disconnect data client in the same thread from where we connected to it
    disconnectDataClient();
}
