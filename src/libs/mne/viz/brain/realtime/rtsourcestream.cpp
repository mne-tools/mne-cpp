//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcestream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Thread / timer plumbing tying the source data worker and per-hemisphere interpolation workers together.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcestream.h"

#include "rtsourcedataworker.h"
#include "rtsourceinterpolationmatworker.h"
#include "periodictask.h"
#include <mne/fs/fs_label.h>
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <utility>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceStream::RtSourceStream()
{
    // Create data worker
    // streamData() runs on the streaming thread, which also calls the result callbacks.
    m_pWorker = std::make_unique<RtSourceDataWorker>();

    // Forward worker results to the callbacks of this stream
    m_pWorker->setNewRtSmoothedDataCallback([this](const std::vector<uint32_t>& colorsLh, const std::vector<uint32_t>& colorsRh) {
        if (m_newSmoothedDataAvailableCallback) {
            m_newSmoothedDataAvailableCallback(colorsLh, colorsRh);
        }
    });
    m_pWorker->setNewRtRawDataCallback([this](const Eigen::VectorXd& dataLh, const Eigen::VectorXd& dataRh) {
        if (m_newRawDataAvailableCallback) {
            m_newRawDataAvailableCallback(dataLh, dataRh);
        }
    });

    // Streaming thread driving the cadence
    m_pStreamTask = std::make_unique<PeriodicTask>([worker = m_pWorker.get()] { worker->streamData(); });

    // Create interpolation matrix worker
    // One pool thread runs the recomputations in request order.
    m_pInterpWorker = std::make_unique<RtSourceInterpolationMatWorker>();
    m_pInterpPool = std::make_unique<mne::ThreadPool>(1);

    // Forward interpolation results: auto-apply to data worker + re-emit
    m_pInterpWorker->setNewInterpolationMatrixLeftAvailableCallback([this](const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat) {
        onNewInterpolationMatrixLeft(interpMat);
    });
    m_pInterpWorker->setNewInterpolationMatrixRightAvailableCallback([this](const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat) {
        onNewInterpolationMatrixRight(interpMat);
    });

    mne::Log::debug("RtSourceStream: Initialized with {} ms interval", m_iTimeInterval);
}

//=============================================================================================================

RtSourceStream::~RtSourceStream()
{
    // Join the threads while the workers are still alive. Pending
    // recomputations are skipped.
    m_bShuttingDown.store(true, std::memory_order_release);
    m_pStreamTask.reset();
    m_pInterpPool.reset();
}

//=============================================================================================================

void RtSourceStream::addData(const Eigen::VectorXd& data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSourceDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSourceStream::setInterpolationMatrixLeft(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(std::move(mat));
    }
}

//=============================================================================================================

void RtSourceStream::setInterpolationMatrixRight(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(std::move(mat));
    }
}

//=============================================================================================================

void RtSourceStream::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pStreamTask->start(std::chrono::milliseconds(m_iTimeInterval));
        mne::Log::debug("RtSourceStream: Streaming started at {} ms interval", m_iTimeInterval);
    } else {
        m_pStreamTask->stop();
        mne::Log::debug("RtSourceStream: Streaming stopped");
    }
}

//=============================================================================================================

bool RtSourceStream::isStreaming() const
{
    return m_bIsStreaming;
}

//=============================================================================================================

void RtSourceStream::setTimeInterval(int msec)
{
    m_iTimeInterval = std::max(1, msec);

    if (m_bIsStreaming) {
        m_pStreamTask->setInterval(std::chrono::milliseconds(m_iTimeInterval));
    }

    mne::Log::debug("RtSourceStream: Time interval set to {} ms", m_iTimeInterval);
}

//=============================================================================================================

void RtSourceStream::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSourceStream::setColormapType(const std::string& name)
{
    if (m_pWorker) {
        m_pWorker->setColormapType(name);
    }
}

//=============================================================================================================

void RtSourceStream::setThresholds(double min, double mid, double max)
{
    if (m_pWorker) {
        m_pWorker->setThresholds(min, mid, max);
    }
}

//=============================================================================================================

void RtSourceStream::setLoopState(bool enabled)
{
    if (m_pWorker) {
        m_pWorker->setLoopState(enabled);
    }
}

//=============================================================================================================

void RtSourceStream::setSFreq(double sFreq)
{
    if (m_pWorker) {
        m_pWorker->setSFreq(sFreq);
    }
}

//=============================================================================================================

void RtSourceStream::clearData()
{
    if (m_pWorker) {
        m_pWorker->clear();
    }
}

//=============================================================================================================

void RtSourceStream::setSurfaceColor(const std::vector<uint32_t>& baseColorsLh,
                                     const std::vector<uint32_t>& baseColorsRh)
{
    if (m_pWorker) {
        m_pWorker->setSurfaceColor(baseColorsLh, baseColorsRh);
    }
}

//=============================================================================================================

void RtSourceStream::setStreamSmoothedData(bool bStreamSmoothedData)
{
    if (m_pWorker) {
        m_pWorker->setStreamSmoothedData(bStreamSmoothedData);
    }
}

//=============================================================================================================

void RtSourceStream::setInterpolationFunction(const std::string& sInterpolationFunction)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationFunction(sInterpolationFunction);
    }
}

//=============================================================================================================

void RtSourceStream::setCancelDistance(double dCancelDist)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setCancelDistance(dCancelDist);
    }
}

//=============================================================================================================

void RtSourceStream::setInterpolationInfoLeft(const Eigen::MatrixX3f& matVertices,
                                              const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                              const Eigen::VectorXi& vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoLeft(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceStream::setInterpolationInfoRight(const Eigen::MatrixX3f& matVertices,
                                               const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                               const Eigen::VectorXi& vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoRight(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceStream::recomputeInterpolation()
{
    if (m_pInterpWorker) {
        m_pInterpPool->submit([this] {
            if (!m_bShuttingDown.load(std::memory_order_acquire)) {
                m_pInterpWorker->computeInterpolationMatrix();
            }
        });
    }
}

//=============================================================================================================

void RtSourceStream::onNewInterpolationMatrixLeft(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(interpMat);
    }
    // Re-emit so external listeners can react
    if (m_newInterpolationMatrixLeftAvailableCallback) {
        m_newInterpolationMatrixLeftAvailableCallback(interpMat);
    }
}

//=============================================================================================================

void RtSourceStream::onNewInterpolationMatrixRight(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(interpMat);
    }
    // Re-emit so external listeners can react
    if (m_newInterpolationMatrixRightAvailableCallback) {
        m_newInterpolationMatrixRightAvailableCallback(interpMat);
    }
}

//=============================================================================================================

void RtSourceStream::setVisualizationType(int iVisType)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setVisualizationType(iVisType);
    }
}

//=============================================================================================================

void RtSourceStream::setAnnotationInfoLeft(const Eigen::VectorXi& vecLabelIds,
                                           const std::vector<mne::FsLabel>& lLabels,
                                           const Eigen::VectorXi& vecVertNo)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setAnnotationInfoLeft(vecLabelIds, lLabels, vecVertNo);
    }
}

//=============================================================================================================

void RtSourceStream::setAnnotationInfoRight(const Eigen::VectorXi& vecLabelIds,
                                            const std::vector<mne::FsLabel>& lLabels,
                                            const Eigen::VectorXi& vecVertNo)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setAnnotationInfoRight(vecLabelIds, lLabels, vecVertNo);
    }
}


//=============================================================================================================

void RtSourceStream::setNewSmoothedDataAvailableCallback(std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> callback)
{
    m_newSmoothedDataAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSourceStream::setNewRawDataAvailableCallback(std::function<void(const Eigen::VectorXd&, const Eigen::VectorXd&)> callback)
{
    m_newRawDataAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSourceStream::setNewInterpolationMatrixLeftAvailableCallback(std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> callback)
{
    m_newInterpolationMatrixLeftAvailableCallback = std::move(callback);
}

//=============================================================================================================

void RtSourceStream::setNewInterpolationMatrixRightAvailableCallback(std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> callback)
{
    m_newInterpolationMatrixRightAvailableCallback = std::move(callback);
}

} // namespace mne::viz
