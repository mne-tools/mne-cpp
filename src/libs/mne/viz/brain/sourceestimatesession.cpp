//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimatesession.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Ownership glue between StcLoadingWorker, SourceEstimateOverlay and RtSourceStream for static and streaming STC.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimatesession.h"

#include <mne/base/log.h>

#include "brainsurface.h"
#include "realtime/rtsourcestream.h"
#include "realtime/stcloadingworker.h"
#include "sourceestimateoverlay.h"
#include "viewstate.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateSession::SourceEstimateSession(Dispatcher dispatcher)
: m_dispatcher(std::move(dispatcher))
{
}

//=============================================================================================================

SourceEstimateSession::~SourceEstimateSession()
{
    cancelLoading();
}

//=============================================================================================================

void SourceEstimateSession::cancelLoading()
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

bool SourceEstimateSession::load(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath,
                                 const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                                 const std::string& activeSurfaceType)
{
    if (m_isLoading) {
        mne::Log::warning("SourceEstimateSession: STC loading already in progress");
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
                    mne::Log::debug("SourceEstimateSession: Using fallback LH surface: {}", key);
                } else if (!rhSurface && key.starts_with("rh_")) {
                    rhSurface = surface.get();
                    mne::Log::debug("SourceEstimateSession: Using fallback RH surface: {}", key);
                }
            }
        }
    }

    if (!lhSurface && !rhSurface) {
        std::string keys;
        for (const auto& [key, surface] : surfaces)
            keys += (keys.empty() ? "" : ", ") + key;
        mne::Log::warning("SourceEstimateSession: No surfaces available for STC loading. Active surface type: {} Available keys: {}", activeSurfaceType, keys);
        return false;
    }

    // Clean up any previous loading thread and worker
    cancelLoading();

    // Create overlay for results
    m_overlay = std::make_unique<SourceEstimateOverlay>();

    // process() runs on the loading thread; its progress and result are handled on the owner's thread
    m_stcWorker = std::make_unique<StcLoadingWorker>(lhPath, rhPath, lhSurface, rhSurface);

    const std::uint64_t generation = m_loadGeneration;
    m_stcWorker->setProgressCallback([this](int percent, const std::string& message) {
        m_dispatcher([this, percent, message] {
            if (m_loadingProgressCallback) {
                m_loadingProgressCallback(percent, message);
            }
        });
    });
    m_stcWorker->setFinishedCallback([this, generation](bool success) {
        m_dispatcher([this, generation, success] {
            if (generation == m_loadGeneration)
                onStcLoadingFinished(success);
        });
    });

    m_isLoading = true;
    m_loadingThread = std::jthread([worker = m_stcWorker.get()] { worker->process(); });

    return true;
}

//=============================================================================================================

bool SourceEstimateSession::isLoaded() const
{
    return m_overlay && m_overlay->isLoaded();
}

//=============================================================================================================

void SourceEstimateSession::onStcLoadingFinished(bool success)
{
    m_isLoading = false;

    if (!success || !m_stcWorker) {
        mne::Log::warning("SourceEstimateSession: Async STC loading failed");
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
    if (m_thresholdsUpdatedCallback) {
        m_thresholdsUpdatedCallback(m_overlay->thresholdMin(),
                                    m_overlay->thresholdMid(),
                                    m_overlay->thresholdMax());
    }

    if (m_overlay->isLoaded()) {
        if (m_loadedCallback) {
            m_loadedCallback(m_overlay->numTimePoints());
        }
    } else {
        m_overlay.reset();
    }
}

//=============================================================================================================

void SourceEstimateSession::setTimePoint(int index,
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

    if (m_timePointChangedCallback) {
        m_timePointChangedCallback(m_currentTimePoint, m_overlay->timeAtIndex(m_currentTimePoint));
    }
}

//=============================================================================================================

float SourceEstimateSession::tstep() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tstep() : 0.0f;
}

//=============================================================================================================

float SourceEstimateSession::tmin() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tmin() : 0.0f;
}

//=============================================================================================================

int SourceEstimateSession::numTimePoints() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->numTimePoints() : 0;
}

//=============================================================================================================

int SourceEstimateSession::closestIndex(float timeSec) const
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

void SourceEstimateSession::setColormap(const std::string& name)
{
    if (m_overlay)
        m_overlay->setColormap(name);
}

//=============================================================================================================

void SourceEstimateSession::setThresholds(float min, float mid, float max)
{
    if (m_overlay)
        m_overlay->setThresholds(min, mid, max);

    if (m_rtStream)
        m_rtStream->setThresholds(min, mid, max);
}

//=============================================================================================================

void SourceEstimateSession::startStreaming([[maybe_unused]] const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                                           [[maybe_unused]] const SubView& singleView,
                                           [[maybe_unused]] const std::vector<SubView>& subViews)
{
    if (m_isStreaming) {
        mne::Log::debug("SourceEstimateSession: Real-time streaming already active");
        return;
    }

    if (!m_overlay || !m_overlay->isLoaded()) {
        mne::Log::warning("SourceEstimateSession: Cannot start streaming — no source estimate loaded");
        return;
    }

    // Create controller on first use
    if (!m_rtStream) {
        m_rtStream = std::make_unique<RtSourceStream>();
        m_rtStream->setNewSmoothedDataAvailableCallback([this](const std::vector<uint32_t>& colorsLh, const std::vector<uint32_t>& colorsRh) {
            m_dispatcher([this, colorsLh, colorsRh] {
                if (m_realtimeColorsAvailableCallback) {
                    m_realtimeColorsAvailableCallback(colorsLh, colorsRh);
                }
            });
        });
    }

    // Propagate interpolation matrices from the overlay
    m_rtStream->setInterpolationMatrixLeft(m_overlay->interpolationMatLh());
    m_rtStream->setInterpolationMatrixRight(m_overlay->interpolationMatRh());

    // Propagate current visualization parameters
    m_rtStream->setColormapType(m_overlay->colormap());
    m_rtStream->setThresholds(m_overlay->thresholdMin(),
                              m_overlay->thresholdMid(),
                              m_overlay->thresholdMax());
    m_rtStream->setSFreq(1.0 / m_overlay->tstep());

    // Feed all STC time-points into the queue
    const int nTimePoints = m_overlay->numTimePoints();
    mne::Log::debug("SourceEstimateSession: Feeding {} time points into real-time queue", nTimePoints);
    m_rtStream->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXd col = m_overlay->sourceDataColumn(t);
        if (col.size() > 0)
            m_rtStream->addData(col);
    }

    m_rtStream->setStreamingState(true);
    m_isStreaming = true;

    mne::Log::debug("SourceEstimateSession: Real-time streaming started");
}

//=============================================================================================================

void SourceEstimateSession::stopStreaming()
{
    if (!m_isStreaming)
        return;

    if (m_rtStream)
        m_rtStream->setStreamingState(false);

    m_isStreaming = false;
    mne::Log::debug("SourceEstimateSession: Real-time streaming stopped");
}

//=============================================================================================================

void SourceEstimateSession::pushData(const Eigen::VectorXd& data)
{
    if (m_rtStream)
        m_rtStream->addData(data);
}

//=============================================================================================================

void SourceEstimateSession::setInterval(int msec)
{
    if (m_rtStream)
        m_rtStream->setTimeInterval(msec);
}

//=============================================================================================================

void SourceEstimateSession::setLooping(bool enabled)
{
    if (m_rtStream)
        m_rtStream->setLoopState(enabled);
}

//=============================================================================================================

const SourceEstimateOverlay* SourceEstimateSession::overlay() const
{
    return m_overlay.get();
}


//=============================================================================================================

void SourceEstimateSession::setLoadedCallback(std::function<void(int)> callback)
{
    m_loadedCallback = std::move(callback);
}

//=============================================================================================================

void SourceEstimateSession::setThresholdsUpdatedCallback(std::function<void(float, float, float)> callback)
{
    m_thresholdsUpdatedCallback = std::move(callback);
}

//=============================================================================================================

void SourceEstimateSession::setTimePointChangedCallback(std::function<void(int, float)> callback)
{
    m_timePointChangedCallback = std::move(callback);
}

//=============================================================================================================

void SourceEstimateSession::setLoadingProgressCallback(std::function<void(int, const std::string&)> callback)
{
    m_loadingProgressCallback = std::move(callback);
}

//=============================================================================================================

void SourceEstimateSession::setRealtimeColorsAvailableCallback(std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> callback)
{
    m_realtimeColorsAvailableCallback = std::move(callback);
}

} // namespace mne::viz
