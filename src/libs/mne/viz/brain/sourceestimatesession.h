//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimatesession.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Owns the source-time-course overlay together with its loader, real-time controller and target cortical surfaces.
 *
 * SourceEstimateSession couples @ref mne::viz::SourceEstimateOverlay (the
 * static colour-mapped renderable), @ref mne::viz::StcLoadingWorker (background
 * loader for @c .stc files) and @ref mne::viz::RtSourceStream (the
 * real-time pipeline) into a single object that the GUI talks to.
 * It exposes time-point scrubbing, playback, looping and threshold /
 * colormap selection without leaking the worker-thread plumbing.
 */

#ifndef VIZ_SOURCEESTIMATESESSION_H
#define VIZ_SOURCEESTIMATESESSION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../host/dispatcher.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


namespace mne::viz
{
class BrainSurface;
} // namespace mne::viz

namespace mne::viz
{
struct SubView;
} // namespace mne::viz

namespace mne::viz
{
class SourceEstimateOverlay;
} // namespace mne::viz

namespace mne::viz
{
class StcLoadingWorker;
} // namespace mne::viz

namespace mne::viz
{
class RtSourceStream;
//=============================================================================================================
/**
 * Manages source estimate data: async loading, time-point navigation,
 * colormap/threshold control, and real-time streaming via RtSourceStream.
 *
 * BrainView delegates all source-estimate operations to this component,
 * keeping its own code free of STC lifecycle details.
 *
 * @brief    Source estimate lifecycle manager.
 */
class VIZSHARED_EXPORT SourceEstimateSession
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] dispatcher   Hands the results of the loading and streaming threads to the owner's thread.
     */
    explicit SourceEstimateSession(Dispatcher dispatcher = immediate_dispatcher());

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~SourceEstimateSession();

    SourceEstimateSession(const SourceEstimateSession&) = delete;
    SourceEstimateSession& operator=(const SourceEstimateSession&) = delete;

    // ── Loading ────────────────────────────────────────────────────────

    /**
     * Start async STC file loading on a background thread.
     *
     * @param[in] lhPath             Path to left-hemisphere STC file.
     * @param[in] rhPath             Path to right-hemisphere STC file.
     * @param[in] surfaces           FsSurface map for finding lh/rh brain surfaces.
     * @param[in] activeSurfaceType  Active surface type name (e.g. "pial").
     * @return true if loading was started, false on error.
     */
    bool load(const std::filesystem::path& lhPath, const std::filesystem::path& rhPath,
              const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
              const std::string& activeSurfaceType);

    /**
     * Cancel any in-progress STC loading / interpolation and wait for the thread to finish.
     */
    void cancelLoading();

    /**
     * @return true while an async STC load is in progress.
     */
    bool isLoading() const
    {
        return m_isLoading;
    }

    /**
     * @return true if source estimate data has been loaded successfully.
     */
    bool isLoaded() const;

    // ── Time-point navigation ──────────────────────────────────────────

    /**
     * Set the current STC time-point and apply colours to matching surfaces.
     *
     * @param[in] index      Time-point index.
     * @param[in] surfaces   FsSurface map.
     * @param[in] singleView Single-view state (for active surface type).
     * @param[in] subViews   Multi-view states.
     */
    void setTimePoint(int index,
                      const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                      const SubView& singleView,
                      const std::vector<SubView>& subViews);

    /** @return current time-point index. */
    int currentTimePoint() const
    {
        return m_currentTimePoint;
    }

    /** @return time step between STC samples (seconds), or 0. */
    float tstep() const;

    /** @return start time of the STC data (seconds), or 0. */
    float tmin() const;

    /** @return number of time points, or 0. */
    int numTimePoints() const;

    /**
     * @param[in] timeSec   Time in seconds.
     * @return index closest to @p timeSec, or -1.
     */
    int closestIndex(float timeSec) const;

    // ── Colormap / thresholds ──────────────────────────────────────────

    /**
     * Set the colormap used for source estimate visualisation.
     *
     * @param[in] name   Colormap name (e.g. "Hot", "Jet").
     */
    void setColormap(const std::string& name);

    /**
     * Set source estimate thresholds.
     *
     * @param[in] min   Minimum threshold.
     * @param[in] mid   Mid threshold.
     * @param[in] max   Maximum threshold.
     */
    void setThresholds(float min, float mid, float max);

    // ── Real-time streaming ────────────────────────────────────────────

    /**
     * Start real-time playback.  Feeds all STC time-points into the
     * streaming queue and begins the interpolation pipeline.
     *
     * @param[in] surfaces   FsSurface map (for colour application).
     * @param[in] singleView Single-view state.
     * @param[in] subViews   Multi-view states.
     */
    void startStreaming(const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                        const SubView& singleView,
                        const std::vector<SubView>& subViews);

    /** Stop real-time streaming. */
    void stopStreaming();

    /** @return true while real-time streaming is active. */
    bool isStreaming() const
    {
        return m_isStreaming;
    }

    /**
     * Push a single column of source data into the streaming queue.
     *
     * @param[in] data   Source amplitudes for one time point; ignored if no controller exists.
     */
    void pushData(const Eigen::VectorXd& data);

    /**
     * Set the streaming playback interval in milliseconds.
     *
     * @param[in] msec   Interval between streamed time points in milliseconds.
     */
    void setInterval(int msec);

    /**
     * Enable or disable looping of the streaming queue.
     *
     * @param[in] enabled   True to replay the queue from the start when it ends.
     */
    void setLooping(bool enabled);

    // ── Accessors ──────────────────────────────────────────────────────

    /** @return read-only pointer to the overlay (may be null). */
    const SourceEstimateOverlay* overlay() const;

public:
    //=========================================================================================================
    /**
     * Set the callback for loaded. Called for loaded. It runs on the owner's thread.
     *
     * @param[in] callback   Receives (numTimePoints); empty to drop the results.
     */
    void setLoadedCallback(std::function<void(int)> callback);

    //=========================================================================================================
    /**
     * Set the callback for thresholdsUpdated. Called for thresholdsUpdated. It runs on the owner's thread.
     *
     * @param[in] callback   Receives (min, mid, max); empty to drop the results.
     */
    void setThresholdsUpdatedCallback(std::function<void(float, float, float)> callback);

    //=========================================================================================================
    /**
     * Set the callback for timePointChanged. Called for timePointChanged. It runs on the owner's thread.
     *
     * @param[in] callback   Receives (index, time); empty to drop the results.
     */
    void setTimePointChangedCallback(std::function<void(int, float)> callback);

    //=========================================================================================================
    /**
     * Set the callback for loadingProgress. Called for loadingProgress. It runs on the owner's thread.
     *
     * @param[in] callback   Receives (percent, message); empty to drop the results.
     */
    void setLoadingProgressCallback(std::function<void(int, const std::string&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for realtimeColorsAvailable. Called when the real-time pipeline produces a new set of per-vertex colours.  The host widget should apply these to its surfaces and repaint. It runs on the owner's thread.
     *
     * @param[in] callback   Receives (colorsLh, colorsRh); empty to drop the results.
     */
    void setRealtimeColorsAvailableCallback(std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> callback);

private:
    /** Handle background-thread STC loading completion. */
    void onStcLoadingFinished(bool success);

private:
    std::unique_ptr<SourceEstimateOverlay> m_overlay; /**< Source estimate data / interpolation. */
    std::unique_ptr<RtSourceStream> m_rtStream;       /**< Real-time source stream. */
    Dispatcher m_dispatcher;                          /**< Hands results to the owner's thread. */
    std::jthread m_loadingThread;                     /**< Background thread for STC file loading. */
    std::uint64_t m_loadGeneration = 0;               /**< Bumped per cancel; stale finished signals are ignored. */
    std::unique_ptr<StcLoadingWorker> m_stcWorker;    /**< Worker performing the async STC load. */
    int m_currentTimePoint = 0;                       /**< Current time-point index. */
    bool m_isLoading = false;                         /**< True while async load is in progress. */
    bool m_isStreaming = false;                       /**< True while real-time streaming is active. */

    std::function<void(int)> m_loadedCallback;                                                                         /**< Receives loaded. */
    std::function<void(float, float, float)> m_thresholdsUpdatedCallback;                                              /**< Receives thresholdsUpdated. */
    std::function<void(int, float)> m_timePointChangedCallback;                                                        /**< Receives timePointChanged. */
    std::function<void(int, const std::string&)> m_loadingProgressCallback;                                            /**< Receives loadingProgress. */
    std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> m_realtimeColorsAvailableCallback; /**< Receives realtimeColorsAvailable. */
};

} // namespace mne::viz

#endif // VIZ_SOURCEESTIMATESESSION_H
