//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulatorproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    Definition of the FiffSimulatorProducer class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulatorproducer.h"
#include "fiffsimulator.h"

#include <fiff/fiff_digitizer_data.h>

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
using namespace COMLIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulatorProducer::FiffSimulatorProducer(FiffSimulator* p_pFiffSimulator)
: m_pFiffSimulator(p_pFiffSimulator)
, m_bDataClientIsConnected(false)
, m_bFlagInfoRequest(false)
, m_iDataClientId(-1)
, m_iDefaultPortDataClient(4218)
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

    m_pRtDataClient->connectToHost(p_sRtSeverIP, m_iDefaultPortDataClient);
    m_pRtDataClient->waitForConnected(1000);

    if(m_pRtDataClient->state() == QTcpSocket::ConnectedState) {
        m_producerMutex.lock();
        if(!m_bDataClientIsConnected) {
            // get client ID
            m_iDataClientId = m_pRtDataClient->getClientId();

            // set data client alias -> for convinience (optional)
            m_pRtDataClient->setClientAlias(m_pFiffSimulator->m_sFiffSimulatorClientAlias); // used in option 2 later on

            // set new state
            m_bDataClientIsConnected = true;
            emit dataConnectionChanged(m_bDataClientIsConnected);
        }
        m_producerMutex.unlock();
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

        m_producerMutex.lock();
        m_iDataClientId = -1;
        m_bDataClientIsConnected = false;
        m_producerMutex.unlock();
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
    while(!isInterruptionRequested() && (m_pRtDataClient->state() != QTcpSocket::ConnectedState)) {
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

    while(!isInterruptionRequested()) {
        m_producerMutex.lock();
        if(m_bFlagInfoRequest) {
            m_pFiffSimulator->m_qMutex.lock();
            auto metadata = m_pRtDataClient->readMetadata();
            m_pFiffSimulator->m_pFiffInfo = metadata.m_pInfo;
            m_pFiffSimulator->m_pFiffDigData = metadata.m_pDigitizerData;
            m_pFiffSimulator->m_qMutex.unlock();
            emit m_pFiffSimulator->fiffInfoAvailable();
            m_bFlagInfoRequest = false;
        }
        m_producerMutex.unlock();

        // Only perform data reading if the measurement was started
        if(m_pFiffSimulator->isRunning()) {
            m_pRtDataClient->readRawBuffer(m_pFiffSimulator->m_pFiffInfo->nchan,
                                           matData,
                                           kind);

            if(kind == FIFF_DATA_BUFFER) {
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
