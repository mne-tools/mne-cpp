//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimatemanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Ownership glue between StcLoadingWorker, SourceEstimateOverlay and RtSourceDataController for static and streaming STC.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimatemanager.h"

#include <mne/base/log.h>

#include "../renderable/sourceestimateoverlay.h"
#include "../renderable/brainsurface.h"
#include "../workers/stcloadingworker.h"
#include "../workers/rtsourcedatacontroller.h"
#include "../core/viewstate.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateManager::SourceEstimateManager(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

SourceEstimateManager::~SourceEstimateManager()
{
    cancelLoading();
}

//=============================================================================================================

void SourceEstimateManager::cancelLoading()
{
    if (m_stcWorker)
        m_stcWorker->requestCancel();
    if (m_loadingThread.joinable())
        m_loadingThread.join();
    // The worker is deleted here, once its thread has stopped, never by the thread itself:
    // onStcLoadingFinished still reads it after the thread may have finished. A finished signal it
    // already queued is dropped by the generation check in load().
    m_stcWorker.reset();
    ++m_loadGeneration;
    m_isLoading = false;
}

//=============================================================================================================

bool SourceEstimateManager::load(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath,
                                 const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                                 const std::string& activeSurfaceType)
{
    if (m_isLoading) {
        mne::Log::warning("SourceEstimateManager: STC loading already in progress");
        return false;
    }

    // Find surfaces for the active surface type
    BrainSurface* lhSurface = nullptr;
    BrainSurface* rhSurface = nullptr;

    const std::string lhKey = "lh_" + activeSurfaceType;
    const std::string rhKey = "rh_" + activeSurfaceType;

    if (const auto it = surfaces.find(lhKey); it != surfaces.end())
        lhSurface = it->second.get();
    if (const auto it = surfaces.find(rhKey); it != surfaces.end())
        rhSurface = it->second.get();

    // Fallback: search for any lh_*/rh_* brain surface
    if (!lhSurface || !rhSurface) {
        for (const auto& [key, surface] : surfaces) {
            if (surface && surface->tissueType() == BrainSurface::TissueBrain) {
                if (!lhSurface && key.starts_with("lh_")) {
                    lhSurface = surface.get();
                    mne::Log::debug("SourceEstimateManager: Using fallback LH surface: {}", key);
                } else if (!rhSurface && key.starts_with("rh_")) {
                    rhSurface = surface.get();
                    mne::Log::debug("SourceEstimateManager: Using fallback RH surface: {}", key);
                }
            }
        }
    }

    if (!lhSurface && !rhSurface) {
        std::string keys;
        for (const auto& [key, surface] : surfaces)
            keys += (keys.empty() ? "" : ", ") + key;
        mne::Log::warning("SourceEstimateManager: No surfaces available for STC loading. Active surface type: {} Available keys: {}", activeSurfaceType, keys);
        return false;
    }

    // Clean up any previous loading thread and worker
    cancelLoading();

    // Create overlay for results
    m_overlay = std::make_unique<SourceEstimateOverlay>();

    // The worker stays on the GUI thread; process() runs on the loading thread, so its signals arrive queued
    m_stcWorker = std::make_unique<StcLoadingWorker>(lhPath, rhPath, lhSurface, rhSurface);

    const std::uint64_t generation = m_loadGeneration;
    connect(m_stcWorker.get(), &StcLoadingWorker::progress, this, &SourceEstimateManager::loadingProgress);
    connect(m_stcWorker.get(), &StcLoadingWorker::finished, this, [this, generation](bool success) {
        if (generation == m_loadGeneration)
            onStcLoadingFinished(success);
    });

    m_isLoading = true;
    m_loadingThread = std::jthread([worker = m_stcWorker.get()] { worker->process(); });

    return true;
}

//=============================================================================================================

bool SourceEstimateManager::isLoaded() const
{
    return m_overlay && m_overlay->isLoaded();
}

//=============================================================================================================

void SourceEstimateManager::onStcLoadingFinished(bool success)
{
    m_isLoading = false;

    if (!success || !m_stcWorker) {
        mne::Log::warning("SourceEstimateManager: Async STC loading failed");
        m_overlay.reset();
        return;
    }

    // Transfer data from worker to overlay
    if (m_stcWorker->hasLh()) {
        m_overlay->setStcData(m_stcWorker->stcLh(), 0);
        if (m_stcWorker->interpolationMatLh())
            m_overlay->setInterpolationMatrix(m_stcWorker->interpolationMatLh(), 0);
    }

    if (m_stcWorker->hasRh()) {
        m_overlay->setStcData(m_stcWorker->stcRh(), 1);
        if (m_stcWorker->interpolationMatRh())
            m_overlay->setInterpolationMatrix(m_stcWorker->interpolationMatRh(), 1);
    }

    m_overlay->updateThresholdsFromData();
    emit thresholdsUpdated(m_overlay->thresholdMin(),
                           m_overlay->thresholdMid(),
                           m_overlay->thresholdMax());

    if (m_overlay->isLoaded()) {
        emit loaded(m_overlay->numTimePoints());
    } else {
        m_overlay.reset();
    }
}

//=============================================================================================================

void SourceEstimateManager::setTimePoint(int index,
                                         const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                                         const SubView& singleView,
                                         const std::vector<SubView>& subViews)
{
    if (!m_overlay || !m_overlay->isLoaded())
        return;

    m_currentTimePoint = std::clamp(index, 0, std::max(0, m_overlay->numTimePoints() - 1));

    // Collect all distinct surface types used across single + multi views
    std::set<std::string> activeTypes;
    activeTypes.insert(singleView.surfaceType);
    for (const SubView& subView : subViews)
        activeTypes.insert(subView.surfaceType);

    // Apply source estimate to surfaces matching ANY active type
    for (const auto& [key, surface] : surfaces) {
        for (const std::string& type : activeTypes) {
            if (key.ends_with(type)) {
                m_overlay->applyToSurface(surface.get(), m_currentTimePoint);
                break;
            }
        }
    }

    emit timePointChanged(m_currentTimePoint, m_overlay->timeAtIndex(m_currentTimePoint));
}

//=============================================================================================================

float SourceEstimateManager::tstep() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tstep() : 0.0f;
}

//=============================================================================================================

float SourceEstimateManager::tmin() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tmin() : 0.0f;
}

//=============================================================================================================

int SourceEstimateManager::numTimePoints() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->numTimePoints() : 0;
}

//=============================================================================================================

int SourceEstimateManager::closestIndex(float timeSec) const
{
    if (!m_overlay || !m_overlay->isLoaded())
        return -1;

    const float t0 = m_overlay->tmin();
    const float dt = m_overlay->tstep();
    const int numPts = m_overlay->numTimePoints();
    if (numPts <= 0 || dt <= 0.0f)
        return -1;

    const int idx = static_cast<int>(std::lround((timeSec - t0) / dt));
    return std::clamp(idx, 0, numPts - 1);
}

//=============================================================================================================

void SourceEstimateManager::setColormap(const std::string& name)
{
    if (m_overlay)
        m_overlay->setColormap(name);
}

//=============================================================================================================

void SourceEstimateManager::setThresholds(float min, float mid, float max)
{
    if (m_overlay)
        m_overlay->setThresholds(min, mid, max);

    if (m_rtController)
        m_rtController->setThresholds(min, mid, max);
}

//=============================================================================================================

void SourceEstimateManager::startStreaming([[maybe_unused]] const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                                           [[maybe_unused]] const SubView& singleView,
                                           [[maybe_unused]] const std::vector<SubView>& subViews)
{
    if (m_isStreaming) {
        mne::Log::debug("SourceEstimateManager: Real-time streaming already active");
        return;
    }

    if (!m_overlay || !m_overlay->isLoaded()) {
        mne::Log::warning("SourceEstimateManager: Cannot start streaming — no source estimate loaded");
        return;
    }

    // Create controller on first use
    if (!m_rtController) {
        m_rtController = std::make_unique<RtSourceDataController>(this);
        connect(m_rtController.get(), &RtSourceDataController::newSmoothedDataAvailable,
                this, &SourceEstimateManager::realtimeColorsAvailable);
    }

    // Propagate interpolation matrices from the overlay
    m_rtController->setInterpolationMatrixLeft(m_overlay->interpolationMatLh());
    m_rtController->setInterpolationMatrixRight(m_overlay->interpolationMatRh());

    // Propagate current visualization parameters
    m_rtController->setColormapType(m_overlay->colormap());
    m_rtController->setThresholds(m_overlay->thresholdMin(),
                                  m_overlay->thresholdMid(),
                                  m_overlay->thresholdMax());
    m_rtController->setSFreq(1.0 / m_overlay->tstep());

    // Feed all STC time-points into the queue
    const int nTimePoints = m_overlay->numTimePoints();
    mne::Log::debug("SourceEstimateManager: Feeding {} time points into real-time queue", nTimePoints);
    m_rtController->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXd col = m_overlay->sourceDataColumn(t);
        if (col.size() > 0)
            m_rtController->addData(col);
    }

    m_rtController->setStreamingState(true);
    m_isStreaming = true;

    mne::Log::debug("SourceEstimateManager: Real-time streaming started");
}

//=============================================================================================================

void SourceEstimateManager::stopStreaming()
{
    if (!m_isStreaming)
        return;

    if (m_rtController)
        m_rtController->setStreamingState(false);

    m_isStreaming = false;
    mne::Log::debug("SourceEstimateManager: Real-time streaming stopped");
}

//=============================================================================================================

void SourceEstimateManager::pushData(const Eigen::VectorXd& data)
{
    if (m_rtController)
        m_rtController->addData(data);
}

//=============================================================================================================

void SourceEstimateManager::setInterval(int msec)
{
    if (m_rtController)
        m_rtController->setTimeInterval(msec);
}

//=============================================================================================================

void SourceEstimateManager::setLooping(bool enabled)
{
    if (m_rtController)
        m_rtController->setLoopState(enabled);
}

//=============================================================================================================

const SourceEstimateOverlay* SourceEstimateManager::overlay() const
{
    return m_overlay.get();
}

} // namespace mne::viz::qt
