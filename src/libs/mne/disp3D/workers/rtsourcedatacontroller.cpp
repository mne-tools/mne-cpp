//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcedatacontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Thread / timer plumbing tying the source data worker and per-hemisphere interpolation workers together.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedatacontroller.h"

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

namespace mne::disp
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataController::RtSourceDataController(QObject* parent)
: QObject(parent)
{
    // Create data worker
    // The worker stays on the GUI thread; streamData() runs on the streaming thread, so its signals reach
    // this controller queued.
    m_pWorker = new mne::disp::RtSourceDataWorker(this);

    // Forward worker signals to controller signals
    connect(m_pWorker, &mne::disp::RtSourceDataWorker::newRtSmoothedData,
            this, &RtSourceDataController::newSmoothedDataAvailable);
    connect(m_pWorker, &mne::disp::RtSourceDataWorker::newRtRawData,
            this, &RtSourceDataController::newRawDataAvailable);

    // Streaming thread driving the cadence
    m_pStreamTask = std::make_unique<PeriodicTask>([worker = m_pWorker] { worker->streamData(); });

    // Create interpolation matrix worker
    // One pool thread runs the recomputations in request order.
    m_pInterpWorker = new mne::disp::RtSourceInterpolationMatWorker(this);
    m_pInterpPool = std::make_unique<mne::ThreadPool>(1);

    // Forward interpolation results: auto-apply to data worker + re-emit
    connect(m_pInterpWorker, &mne::disp::RtSourceInterpolationMatWorker::newInterpolationMatrixLeftAvailable,
            this, &RtSourceDataController::onNewInterpolationMatrixLeft);
    connect(m_pInterpWorker, &mne::disp::RtSourceInterpolationMatWorker::newInterpolationMatrixRightAvailable,
            this, &RtSourceDataController::onNewInterpolationMatrixRight);

    mne::Log::debug("RtSourceDataController: Initialized with {} ms interval", m_iTimeInterval);
}

//=============================================================================================================

RtSourceDataController::~RtSourceDataController()
{
    // Join the threads while the workers (children of this object) are still alive. Pending
    // recomputations are skipped.
    m_bShuttingDown.store(true, std::memory_order_release);
    m_pStreamTask.reset();
    m_pInterpPool.reset();
}

//=============================================================================================================

void RtSourceDataController::addData(const Eigen::VectorXd& data)
{
    if (m_pWorker) {
        // Direct call is thread-safe because RtSourceDataWorker uses a mutex
        m_pWorker->addData(data);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationMatrixLeft(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(std::move(mat));
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationMatrixRight(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(std::move(mat));
    }
}

//=============================================================================================================

void RtSourceDataController::setStreamingState(bool state)
{
    m_bIsStreaming = state;

    if (state) {
        m_pStreamTask->start(std::chrono::milliseconds(m_iTimeInterval));
        mne::Log::debug("RtSourceDataController: Streaming started at {} ms interval", m_iTimeInterval);
    } else {
        m_pStreamTask->stop();
        mne::Log::debug("RtSourceDataController: Streaming stopped");
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
        m_pStreamTask->setInterval(std::chrono::milliseconds(m_iTimeInterval));
    }

    mne::Log::debug("RtSourceDataController: Time interval set to {} ms", m_iTimeInterval);
}

//=============================================================================================================

void RtSourceDataController::setNumberAverages(int numAvr)
{
    if (m_pWorker) {
        m_pWorker->setNumberAverages(numAvr);
    }
}

//=============================================================================================================

void RtSourceDataController::setColormapType(const std::string& name)
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

void RtSourceDataController::setSurfaceColor(const std::vector<uint32_t>& baseColorsLh,
                                             const std::vector<uint32_t>& baseColorsRh)
{
    if (m_pWorker) {
        m_pWorker->setSurfaceColor(baseColorsLh, baseColorsRh);
    }
}

//=============================================================================================================

void RtSourceDataController::setStreamSmoothedData(bool bStreamSmoothedData)
{
    if (m_pWorker) {
        m_pWorker->setStreamSmoothedData(bStreamSmoothedData);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationFunction(const std::string& sInterpolationFunction)
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

void RtSourceDataController::setInterpolationInfoLeft(const Eigen::MatrixX3f& matVertices,
                                                      const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                                      const Eigen::VectorXi& vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoLeft(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceDataController::setInterpolationInfoRight(const Eigen::MatrixX3f& matVertices,
                                                       const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                                       const Eigen::VectorXi& vecSourceVertices)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setInterpolationInfoRight(matVertices, vecNeighborVertices, vecSourceVertices);
    }
}

//=============================================================================================================

void RtSourceDataController::recomputeInterpolation()
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

void RtSourceDataController::onNewInterpolationMatrixLeft(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixLeft(interpMat);
    }
    // Re-emit so external listeners can react
    emit newInterpolationMatrixLeftAvailable(interpMat);
}

//=============================================================================================================

void RtSourceDataController::onNewInterpolationMatrixRight(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat)
{
    // Auto-forward to data worker so it immediately uses the new matrix
    if (m_pWorker) {
        m_pWorker->setInterpolationMatrixRight(interpMat);
    }
    // Re-emit so external listeners can react
    emit newInterpolationMatrixRightAvailable(interpMat);
}

//=============================================================================================================

void RtSourceDataController::setVisualizationType(int iVisType)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setVisualizationType(iVisType);
    }
}

//=============================================================================================================

void RtSourceDataController::setAnnotationInfoLeft(const Eigen::VectorXi& vecLabelIds,
                                                   const std::vector<mne::FsLabel>& lLabels,
                                                   const Eigen::VectorXi& vecVertNo)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setAnnotationInfoLeft(vecLabelIds, lLabels, vecVertNo);
    }
}

//=============================================================================================================

void RtSourceDataController::setAnnotationInfoRight(const Eigen::VectorXi& vecLabelIds,
                                                    const std::vector<mne::FsLabel>& lLabels,
                                                    const Eigen::VectorXi& vecVertNo)
{
    if (m_pInterpWorker) {
        m_pInterpWorker->setAnnotationInfoRight(vecLabelIds, lLabels, vecVertNo);
    }
}

} // namespace mne::disp
