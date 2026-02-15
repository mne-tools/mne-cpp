//=============================================================================================================
/**
 * @file     rtsensordatacontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    RtSensorDataController class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordatacontroller.h"
#include "rtsensordataworker.h"

#include <QThread>
#include <QTimer>
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataController::RtSensorDataController(QObject *parent)
    : QObject(parent)
{
    // Create worker
    m_pWorker = new DISP3DRHILIB::RtSensorDataWorker();

    // Create and configure thread
    m_pWorkerThread = new QThread(this);
    m_pWorker->moveToThread(m_pWorkerThread);

    // Clean up worker when thread finishes
    connect(m_pWorkerThread, &QThread::finished, m_pWorker, &QObject::deleteLater);

    // Forward worker signals to controller signals
    connect(m_pWorker, &DISP3DRHILIB::RtSensorDataWorker::newRtSensorColors,
            this, &RtSensorDataController::newSensorColorsAvailable);

    // Create timer for driving the streaming cadence
    m_pTimer = new QTimer(this);
    m_pTimer->setTimerType(Qt::PreciseTimer);
    connect(m_pTimer, &QTimer::timeout, m_pWorker, &DISP3DRHILIB::RtSensorDataWorker::streamData);

    // Start the worker thread
    m_pWorkerThread->start();

    qDebug() << "RtSensorDataController: Initialized with" << m_iTimeInterval << "ms interval";
}

//=============================================================================================================

RtSensorDataController::~RtSensorDataController()
{
    // Stop timer
    if (m_pTimer) {
        m_pTimer->stop();
    }

    // Stop the worker thread
    if (m_pWorkerThread) {
        m_pWorkerThread->quit();
        m_pWorkerThread->wait();
    }
}

//=============================================================================================================

void RtSensorDataController::addData(const Eigen::VectorXf &data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSensorDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSensorDataController::setMappingMatrix(QSharedPointer<Eigen::MatrixXf> mat)
{
    if (m_pWorker) {
        m_pWorker->setMappingMatrix(mat);
    }
}

//=============================================================================================================

void RtSensorDataController::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pTimer->start(m_iTimeInterval);
        qDebug() << "RtSensorDataController: Streaming started at" << m_iTimeInterval << "ms interval";
    } else {
        m_pTimer->stop();
        qDebug() << "RtSensorDataController: Streaming stopped";
    }
}

//=============================================================================================================

bool RtSensorDataController::isStreaming() const
{
    return m_bIsStreaming;
}

//=============================================================================================================

void RtSensorDataController::setTimeInterval(int msec)
{
    m_iTimeInterval = qMax(1, msec);

    if (m_bIsStreaming) {
        m_pTimer->setInterval(m_iTimeInterval);
    }

    qDebug() << "RtSensorDataController: Time interval set to" << m_iTimeInterval << "ms";
}

//=============================================================================================================

void RtSensorDataController::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSensorDataController::setColormapType(const QString &name)
{
    if (m_pWorker) {
        m_pWorker->setColormapType(name);
    }
}

//=============================================================================================================

void RtSensorDataController::setLoopState(bool enabled)
{
    if (m_pWorker) {
        m_pWorker->setLoopState(enabled);
    }
}

//=============================================================================================================

void RtSensorDataController::setSFreq(double sFreq)
{
    if (m_pWorker) {
        m_pWorker->setSFreq(sFreq);
    }
}

//=============================================================================================================

void RtSensorDataController::clearData()
{
    if (m_pWorker) {
        m_pWorker->clear();
    }
}
