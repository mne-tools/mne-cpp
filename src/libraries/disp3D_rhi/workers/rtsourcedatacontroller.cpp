//=============================================================================================================
/**
 * @file     rtsourcedatacontroller.cpp
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
 * @brief    RtSourceDataController class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedatacontroller.h"
#include "rtsourcedataworker.h"
#include "rtsourceinterpolationmatworker.h"

#include <QThread>
#include <QTimer>
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataController::RtSourceDataController(QObject *parent)
    : QObject(parent)
{
    // Create data worker
    m_pWorker = new DISP3DRHILIB::RtSourceDataWorker();

    // Create and configure data thread
    m_pWorkerThread = new QThread(this);
    m_pWorker->moveToThread(m_pWorkerThread);

    // Clean up worker when thread finishes
    connect(m_pWorkerThread, &QThread::finished, m_pWorker, &QObject::deleteLater);

    // Forward worker signals to controller signals
    connect(m_pWorker, &DISP3DRHILIB::RtSourceDataWorker::newRtSmoothedData,
            this, &RtSourceDataController::newSmoothedDataAvailable);
    connect(m_pWorker, &DISP3DRHILIB::RtSourceDataWorker::newRtRawData,
            this, &RtSourceDataController::newRawDataAvailable);

    // Create timer for driving the streaming cadence
    m_pTimer = new QTimer(this);
    m_pTimer->setTimerType(Qt::PreciseTimer);
    connect(m_pTimer, &QTimer::timeout, m_pWorker, &DISP3DRHILIB::RtSourceDataWorker::streamData);

    // Start the data worker thread
    m_pWorkerThread->start();

    // Create interpolation matrix worker
    m_pInterpWorker = new DISP3DRHILIB::RtSourceInterpolationMatWorker();
    m_pInterpThread = new QThread(this);
    m_pInterpWorker->moveToThread(m_pInterpThread);

    connect(m_pInterpThread, &QThread::finished, m_pInterpWorker, &QObject::deleteLater);

    // Forward interpolation results: auto-apply to data worker + re-emit
    connect(m_pInterpWorker, &DISP3DRHILIB::RtSourceInterpolationMatWorker::newInterpolationMatrixLeftAvailable,
            this, &RtSourceDataController::onNewInterpolationMatrixLeft);
    connect(m_pInterpWorker, &DISP3DRHILIB::RtSourceInterpolationMatWorker::newInterpolationMatrixRightAvailable,
            this, &RtSourceDataController::onNewInterpolationMatrixRight);

    m_pInterpThread->start();

    qDebug() << "RtSourceDataController: Initialized with" << m_iTimeInterval << "ms interval";
}

//=============================================================================================================

RtSourceDataController::~RtSourceDataController()
{
    // Stop timer
    if (m_pTimer) {
        m_pTimer->stop();
    }

    // Stop the data worker thread
    if (m_pWorkerThread) {
        m_pWorkerThread->quit();
        m_pWorkerThread->wait();
    }

    // Stop the interpolation worker thread
    if (m_pInterpThread) {
        m_pInterpThread->quit();
        m_pInterpThread->wait();
    }
}

//=============================================================================================================

void RtSourceDataController::addData(const Eigen::VectorXd &data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSourceDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(mat);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(mat);
    }
}

//=============================================================================================================

void RtSourceDataController::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pTimer->start(m_iTimeInterval);
        qDebug() << "RtSourceDataController: Streaming started at" << m_iTimeInterval << "ms interval";
    } else {
        m_pTimer->stop();
        qDebug() << "RtSourceDataController: Streaming stopped";
    }
}

//=============================================================================================================

bool RtSourceDataController::isStreaming() const
{
    return m_bIsStreaming;
}

//=============================================================================================================

void RtSourceDataController::setTimeInterval(int msec)
{
    m_iTimeInterval = qMax(1, msec);

    if (m_bIsStreaming) {
        m_pTimer->setInterval(m_iTimeInterval);
    }

    qDebug() << "RtSourceDataController: Time interval set to" << m_iTimeInterval << "ms";
}

//=============================================================================================================

void RtSourceDataController::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSourceDataController::setColormapType(const QString &name)
{
    if (m_pWorker) {
        m_pWorker->setColormapType(name);
    }
}

//=============================================================================================================

void RtSourceDataController::setThresholds(double min, double mid, double max)
{
    if (m_pWorker) {
        m_pWorker->setThresholds(min, mid, max);
    }
}

//=============================================================================================================

void RtSourceDataController::setLoopState(bool enabled)
{
    if (m_pWorker) {
        m_pWorker->setLoopState(enabled);
    }
}

//=============================================================================================================

void RtSourceDataController::setSFreq(double sFreq)
{
    if (m_pWorker) {
        m_pWorker->setSFreq(sFreq);
    }
}

//=============================================================================================================

void RtSourceDataController::clearData()
{
    if (m_pWorker) {
        m_pWorker->clear();
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationFunction(const QString &sInterpolationFunction)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationFunction(sInterpolationFunction);
    }
}

//=============================================================================================================

void RtSourceDataController::setCancelDistance(double dCancelDist)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setCancelDistance(dCancelDist);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationInfoLeft(const Eigen::MatrixX3f &matVertices,
                                                       const QVector<QVector<int>> &vecNeighborVertices,
                                                       const QVector<int> &vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoLeft(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationInfoRight(const Eigen::MatrixX3f &matVertices,
                                                        const QVector<QVector<int>> &vecNeighborVertices,
                                                        const QVector<int> &vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoRight(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceDataController::recomputeInterpolation()
{
    if (m_pInterpWorker) {
        QMetaObject::invokeMethod(m_pInterpWorker, "computeInterpolationMatrix", Qt::QueuedConnection);
    }
}

//=============================================================================================================

void RtSourceDataController::onNewInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(interpMat);
    }
    // Re-emit so external listeners can react
    emit newInterpolationMatrixLeftAvailable(interpMat);
}

//=============================================================================================================

void RtSourceDataController::onNewInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(interpMat);
    }
    // Re-emit so external listeners can react
    emit newInterpolationMatrixRightAvailable(interpMat);
}
