//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorstream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Thread / timer plumbing tying the sensor data worker and field-mapping worker into one streaming pipeline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorstream.h"

#include "rtsensordataworker.h"
#include "rtsensorinterpolationmatworker.h"
#include "periodictask.h"
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorStream::RtSensorStream()
{
    // Create data worker
    // streamData() runs on the streaming thread, which also calls the result callbacks.
    m_pWorker = std::make_unique<RtSensorDataWorker>();

    // Forward worker results to the callbacks of this stream
    m_pWorker->setNewRtSensorColorsCallback([this](const std::string& surfaceKey, const std::vector<uint32_t>& colors) {
        if (m_newSensorColorsAvailableCallback) {
            m_newSensorColorsAvailableCallback(surfaceKey, colors);
        }
    });
    m_pWorker->setNewRtRawSensorDataCallback([this](const Eigen::VectorXf& data) {
        if (m_newRawSensorDataAvailableCallback) {
            m_newRawSensorDataAvailableCallback(data);
        }
    });

    // Streaming thread driving the cadence
    m_pStreamTask = std::make_unique<PeriodicTask>([worker = m_pWorker.get()] { worker->streamData(); });

    // Create interpolation matrix worker
    // One pool thread runs the recomputations in request order.
    m_pInterpWorker = std::make_unique<RtSensorInterpolationMatWorker>();
    m_pInterpPool = std::make_unique<mne::ThreadPool>(1);

    // Forward interpolation results: auto-apply to data worker + re-emit for external listeners
    m_pInterpWorker->setNewMegMappingAvailableCallback([this](const std::string& surfaceKey, const std::shared_ptr<Eigen::MatrixXf>& mappingMat, const std::vector<int>& pick) {
        onNewMegMapping(surfaceKey, mappingMat, pick);
    });
    m_pInterpWorker->setNewEegMappingAvailableCallback([this](const std::string& surfaceKey, const std::shared_ptr<Eigen::MatrixXf>& mappingMat, const std::vector<int>& pick) {
        onNewEegMapping(surfaceKey, mappingMat, pick);
    });

    mne::Log::debug("RtSensorStream: Initialized with {} ms interval", m_iTimeInterval);
}

//=============================================================================================================

RtSensorStream::~RtSensorStream()
{
    // Join the threads while the workers are still alive. Pending
    // recomputations are skipped.
    m_bShuttingDown.store(true, std::memory_order_release);
    m_pStreamTask.reset();
    m_pInterpPool.reset();
}

//=============================================================================================================

void RtSensorStream::addData(const Eigen::VectorXf& data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSensorDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSensorStream::setMappingMatrix(const std::string& surfaceKey, std::shared_ptr<Eigen::MatrixXf> mat)
{
    if (m_pWorker) {
        m_pWorker->setMappingMatrix(surfaceKey, mat);
    }
}

//=============================================================================================================

void RtSensorStream::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pStreamTask->start(std::chrono::milliseconds(m_iTimeInterval));
        mne::Log::debug("RtSensorStream: Streaming started at {} ms interval", m_iTimeInterval);
    } else {
        m_pStreamTask->stop();
        mne::Log::debug("RtSensorStream: Streaming stopped");
    }
}

//=============================================================================================================

bool RtSensorStream::isStreaming() const
{
    return m_bIsStreaming;
}

//=============================================================================================================

void RtSensorStream::setTimeInterval(int msec)
{
    m_iTimeInterval = std::max(1, msec);

    if (m_bIsStreaming) {
        m_pStreamTask->setInterval(std::chrono::milliseconds(m_iTimeInterval));
    }

    mne::Log::debug("RtSensorStream: Time interval set to {} ms", m_iTimeInterval);
}

//=============================================================================================================

void RtSensorStream::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSensorStream::setColormapType(const std::string& name)
{
    if (m_pWorker) {
        m_pWorker->setColormapType(name);
    }
}

//=============================================================================================================

void RtSensorStream::setThresholds(double min, double max)
{
    if (m_pWorker) {
        m_pWorker->setThresholds(min, max);
    }
}

//=============================================================================================================

void RtSensorStream::setLoopState(bool enabled)
{
    if (m_pWorker) {
        m_pWorker->setLoopState(enabled);
    }
}

//=============================================================================================================

void RtSensorStream::setSFreq(double sFreq)
{
    if (m_pWorker) {
        m_pWorker->setSFreq(sFreq);
    }
}

//=============================================================================================================

void RtSensorStream::clearData()
{
    if (m_pWorker) {
        m_pWorker->clear();
    }
}

//=============================================================================================================

void RtSensorStream::setStreamSmoothedData(bool bStreamSmoothedData)
{
    if (m_pWorker) {
        m_pWorker->setStreamSmoothedData(bStreamSmoothedData);
    }
}

//=============================================================================================================
// ── On-the-fly interpolation matrix computation ────────────────────────
//=============================================================================================================

void RtSensorStream::setEvoked(const mne::FiffEvoked& evoked)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setEvoked(evoked);
    }
}

//=============================================================================================================

void RtSensorStream::setTransform(const mne::FiffCoordTrans& trans,
                                  bool applySensorTrans)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setTransform(trans, applySensorTrans);
    }
}

//=============================================================================================================

void RtSensorStream::setMegFieldMapOnHead(bool onHead)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setMegFieldMapOnHead(onHead);
    }
}

//=============================================================================================================

void RtSensorStream::setMegSurface(const std::string& surfaceKey,
                                   const Eigen::MatrixX3f& vertices,
                                   const Eigen::MatrixX3f& normals,
                                   const Eigen::MatrixX3i& triangles)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setMegSurface(surfaceKey, vertices, normals, triangles);
    }
}

//=============================================================================================================

void RtSensorStream::setEegSurface(const std::string& surfaceKey,
                                   const Eigen::MatrixX3f& vertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setEegSurface(surfaceKey, vertices);
    }
}

//=============================================================================================================

void RtSensorStream::setBadChannels(const std::vector<std::string>& bads)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setBadChannels(bads);
    }
}

//=============================================================================================================

void RtSensorStream::recomputeMapping()
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

void RtSensorStream::onNewMegMapping(const std::string& surfaceKey,
                                     std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                     const std::vector<int>& pick)
{
    // Auto-forward the new matrix to the data worker
    if (m_pWorker && mappingMat) {
        m_pWorker->setMappingMatrix(surfaceKey, mappingMat);
    }

    // Re-emit for external listeners (e.g. BrainView)
    if (m_newMegMappingAvailableCallback) {
        m_newMegMappingAvailableCallback(surfaceKey, mappingMat, pick);
    }

    mne::Log::debug("RtSensorStream: New MEG mapping received and forwarded ({}x{})", mappingMat->rows(), mappingMat->cols());
}

//=============================================================================================================

void RtSensorStream::onNewEegMapping(const std::string& surfaceKey,
                                     std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                     const std::vector<int>& pick)
{
    // Auto-forward the new matrix to the data worker
    if (m_pWorker && mappingMat) {
        m_pWorker->setMappingMatrix(surfaceKey, mappingMat);
    }

    // Re-emit for external listeners (e.g. BrainView)
    if (m_newEegMappingAvailableCallback) {
        m_newEegMappingAvailableCallback(surfaceKey, mappingMat, pick);
    }

    mne::Log::debug("RtSensorStream: New EEG mapping received and forwarded ({}x{})", mappingMat->rows(), mappingMat->cols());
}


//=============================================================================================================

void RtSensorStream::setNewSensorColorsAvailableCallback(std::function<void(const std::string&, const std::vector<uint32_t>&)> callback)
{
    m_newSensorColorsAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSensorStream::setNewRawSensorDataAvailableCallback(std::function<void(const Eigen::VectorXf&)> callback)
{
    m_newRawSensorDataAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSensorStream::setNewMegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback)
{
    m_newMegMappingAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSensorStream::setNewEegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback)
{
    m_newEegMappingAvailableCallback = std::move(callback);
}

} // namespace mne::viz
