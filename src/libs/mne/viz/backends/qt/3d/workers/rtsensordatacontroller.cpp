//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensordatacontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Thread / timer plumbing tying the sensor data worker and field-mapping worker into one streaming pipeline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordatacontroller.h"

#include "rtsensordataworker.h"
#include "rtsensorinterpolationmatworker.h"
#include "periodictask.h"
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataController::RtSensorDataController(QObject* parent)
: QObject(parent)
{
    // Create data worker
    // The worker stays on the GUI thread; streamData() runs on the streaming thread, so its signals reach
    // this controller queued.
    m_pWorker = new mne::viz::qt::RtSensorDataWorker(this);

    // Forward worker signals to controller signals
    connect(m_pWorker, &mne::viz::qt::RtSensorDataWorker::newRtSensorColors,
            this, &RtSensorDataController::newSensorColorsAvailable);
    connect(m_pWorker, &mne::viz::qt::RtSensorDataWorker::newRtRawSensorData,
            this, &RtSensorDataController::newRawSensorDataAvailable);

    // Streaming thread driving the cadence
    m_pStreamTask = std::make_unique<PeriodicTask>([worker = m_pWorker] { worker->streamData(); });

    // Create interpolation matrix worker
    // One pool thread runs the recomputations in request order.
    m_pInterpWorker = new mne::viz::qt::RtSensorInterpolationMatWorker(this);
    m_pInterpPool = std::make_unique<mne::ThreadPool>(1);

    // Forward interpolation results: auto-apply to data worker + re-emit for external listeners
    connect(m_pInterpWorker, &mne::viz::qt::RtSensorInterpolationMatWorker::newMegMappingAvailable,
            this, &RtSensorDataController::onNewMegMapping);
    connect(m_pInterpWorker, &mne::viz::qt::RtSensorInterpolationMatWorker::newEegMappingAvailable,
            this, &RtSensorDataController::onNewEegMapping);

    mne::Log::debug("RtSensorDataController: Initialized with {} ms interval", m_iTimeInterval);
}

//=============================================================================================================

RtSensorDataController::~RtSensorDataController()
{
    // Join the threads while the workers (children of this object) are still alive. Pending
    // recomputations are skipped.
    m_bShuttingDown.store(true, std::memory_order_release);
    m_pStreamTask.reset();
    m_pInterpPool.reset();
}

//=============================================================================================================

void RtSensorDataController::addData(const Eigen::VectorXf& data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSensorDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSensorDataController::setMappingMatrix(const std::string& surfaceKey, std::shared_ptr<Eigen::MatrixXf> mat)
{
    if (m_pWorker) {
        m_pWorker->setMappingMatrix(surfaceKey, mat);
    }
}

//=============================================================================================================

void RtSensorDataController::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pStreamTask->start(std::chrono::milliseconds(m_iTimeInterval));
        mne::Log::debug("RtSensorDataController: Streaming started at {} ms interval", m_iTimeInterval);
    } else {
        m_pStreamTask->stop();
        mne::Log::debug("RtSensorDataController: Streaming stopped");
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
        m_pStreamTask->setInterval(std::chrono::milliseconds(m_iTimeInterval));
    }

    mne::Log::debug("RtSensorDataController: Time interval set to {} ms", m_iTimeInterval);
}

//=============================================================================================================

void RtSensorDataController::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSensorDataController::setColormapType(const std::string& name)
{
    if (m_pWorker) {
        m_pWorker->setColormapType(name);
    }
}

//=============================================================================================================

void RtSensorDataController::setThresholds(double min, double max)
{
    if (m_pWorker) {
        m_pWorker->setThresholds(min, max);
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

//=============================================================================================================

void RtSensorDataController::setStreamSmoothedData(bool bStreamSmoothedData)
{
    if (m_pWorker) {
        m_pWorker->setStreamSmoothedData(bStreamSmoothedData);
    }
}

//=============================================================================================================
// ── On-the-fly interpolation matrix computation ────────────────────────
//=============================================================================================================

void RtSensorDataController::setEvoked(const mne::FiffEvoked& evoked)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setEvoked(evoked);
    }
}

//=============================================================================================================

void RtSensorDataController::setTransform(const mne::FiffCoordTrans& trans,
                                          bool applySensorTrans)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setTransform(trans, applySensorTrans);
    }
}

//=============================================================================================================

void RtSensorDataController::setMegFieldMapOnHead(bool onHead)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setMegFieldMapOnHead(onHead);
    }
}

//=============================================================================================================

void RtSensorDataController::setMegSurface(const std::string& surfaceKey,
                                           const Eigen::MatrixX3f& vertices,
                                           const Eigen::MatrixX3f& normals,
                                           const Eigen::MatrixX3i& triangles)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setMegSurface(surfaceKey, vertices, normals, triangles);
    }
}

//=============================================================================================================

void RtSensorDataController::setEegSurface(const std::string& surfaceKey,
                                           const Eigen::MatrixX3f& vertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setEegSurface(surfaceKey, vertices);
    }
}

//=============================================================================================================

void RtSensorDataController::setBadChannels(const std::vector<std::string>& bads)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setBadChannels(bads);
    }
}

//=============================================================================================================

void RtSensorDataController::recomputeMapping()
{
    if (m_pInterpWorker) {
        m_pInterpPool->submit([this] {
            if (!m_bShuttingDown.load(std::memory_order_acquire)) {
                m_pInterpWorker->computeMapping();
            }
        });
    }
}

//=============================================================================================================

void RtSensorDataController::onNewMegMapping(const std::string& surfaceKey,
                                             std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                             const std::vector<int>& pick)
{
    // Auto-forward the new matrix to the data worker
    if (m_pWorker && mappingMat) {
        m_pWorker->setMappingMatrix(surfaceKey, mappingMat);
    }

    // Re-emit for external listeners (e.g. BrainView)
    emit newMegMappingAvailable(surfaceKey, mappingMat, pick);

    mne::Log::debug("RtSensorDataController: New MEG mapping received and forwarded ({}x{})", mappingMat->rows(), mappingMat->cols());
}

//=============================================================================================================

void RtSensorDataController::onNewEegMapping(const std::string& surfaceKey,
                                             std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                             const std::vector<int>& pick)
{
    // Auto-forward the new matrix to the data worker
    if (m_pWorker && mappingMat) {
        m_pWorker->setMappingMatrix(surfaceKey, mappingMat);
    }

    // Re-emit for external listeners (e.g. BrainView)
    emit newEegMappingAvailable(surfaceKey, mappingMat, pick);

    mne::Log::debug("RtSensorDataController: New EEG mapping received and forwarded ({}x{})", mappingMat->rows(), mappingMat->cols());
}

} // namespace mne::viz::qt
