//=============================================================================================================
/**
 * @file     sourceestimatemanager.h
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
 * @brief    SourceEstimateManager class declaration — owns STC overlay, loading, and real-time streaming.
 *
 */

#ifndef SOURCEESTIMATEMANAGER_H
#define SOURCEESTIMATEMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QThread;
class BrainSurface;
class SourceEstimateOverlay;
class StcLoadingWorker;
class RtSourceDataController;
struct SubView;

//=============================================================================================================
/**
 * Manages source estimate data: async loading, time-point navigation,
 * colormap/threshold control, and real-time streaming via RtSourceDataController.
 *
 * BrainView delegates all source-estimate operations to this component,
 * keeping its own code free of STC lifecycle details.
 *
 * @brief    Source estimate lifecycle manager.
 */
class DISP3DRHISHARED_EXPORT SourceEstimateManager : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit SourceEstimateManager(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~SourceEstimateManager() override;

    // ── Loading ────────────────────────────────────────────────────────

    /**
     * Start async STC file loading on a background thread.
     *
     * @param[in] lhPath             Path to left-hemisphere STC file.
     * @param[in] rhPath             Path to right-hemisphere STC file.
     * @param[in] surfaces           Surface map for finding lh/rh brain surfaces.
     * @param[in] activeSurfaceType  Active surface type name (e.g. "pial").
     * @return true if loading was started, false on error.
     */
    bool load(const QString &lhPath, const QString &rhPath,
              const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
              const QString &activeSurfaceType);

    /**
     * @return true while an async STC load is in progress.
     */
    bool isLoading() const { return m_isLoading; }

    /**
     * @return true if source estimate data has been loaded successfully.
     */
    bool isLoaded() const;

    // ── Time-point navigation ──────────────────────────────────────────

    /**
     * Set the current STC time-point and apply colours to matching surfaces.
     *
     * @param[in] index      Time-point index.
     * @param[in] surfaces   Surface map.
     * @param[in] singleView Single-view state (for active surface type).
     * @param[in] subViews   Multi-view states.
     */
    void setTimePoint(int index,
                      const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                      const SubView &singleView,
                      const QVector<SubView> &subViews);

    /** @return current time-point index. */
    int currentTimePoint() const { return m_currentTimePoint; }

    /** @return time step between STC samples (seconds), or 0. */
    float tstep() const;

    /** @return start time of the STC data (seconds), or 0. */
    float tmin() const;

    /** @return number of time points, or 0. */
    int numTimePoints() const;

    /** @return index closest to @p timeSec, or -1. */
    int closestIndex(float timeSec) const;

    // ── Colormap / thresholds ──────────────────────────────────────────

    /**
     * Set the colormap used for source estimate visualisation.
     *
     * @param[in] name   Colormap name (e.g. "Hot", "Jet").
     */
    void setColormap(const QString &name);

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
     * @param[in] surfaces   Surface map (for colour application).
     * @param[in] singleView Single-view state.
     * @param[in] subViews   Multi-view states.
     */
    void startStreaming(const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                        const SubView &singleView,
                        const QVector<SubView> &subViews);

    /** Stop real-time streaming. */
    void stopStreaming();

    /** @return true while real-time streaming is active. */
    bool isStreaming() const { return m_isStreaming; }

    /** Push a single column of source data into the streaming queue. */
    void pushData(const Eigen::VectorXd &data);

    /** Set the streaming playback interval in milliseconds. */
    void setInterval(int msec);

    /** Enable or disable looping of the streaming queue. */
    void setLooping(bool enabled);

    // ── Accessors ──────────────────────────────────────────────────────

    /** @return read-only pointer to the overlay (may be null). */
    const SourceEstimateOverlay *overlay() const;

signals:
    //=========================================================================================================
    /** Emitted when STC data finishes loading. */
    void loaded(int numTimePoints);

    /** Emitted when thresholds are auto-computed from data. */
    void thresholdsUpdated(float min, float mid, float max);

    /** Emitted when the active time point changes. */
    void timePointChanged(int index, float time);

    /** Emitted during async loading to report progress. */
    void loadingProgress(int percent, const QString &message);

    /**
     * Emitted when the real-time pipeline produces a new set of
     * per-vertex colours.  The host widget should apply these to its
     * surfaces and repaint.
     */
    void realtimeColorsAvailable(const QVector<uint32_t> &colorsLh,
                                  const QVector<uint32_t> &colorsRh);

private slots:
    /** Handle background-thread STC loading completion. */
    void onStcLoadingFinished(bool success);

private:
    std::unique_ptr<SourceEstimateOverlay> m_overlay;       /**< Source estimate data / interpolation. */
    std::unique_ptr<RtSourceDataController> m_rtController; /**< Real-time streaming controller. */
    QThread *m_loadingThread = nullptr;                     /**< Background thread for STC file loading. */
    StcLoadingWorker *m_stcWorker = nullptr;                /**< Worker performing the async STC load. */
    int m_currentTimePoint = 0;                             /**< Current time-point index. */
    bool m_isLoading = false;                               /**< True while async load is in progress. */
    bool m_isStreaming = false;                              /**< True while real-time streaming is active. */
};

#endif // SOURCEESTIMATEMANAGER_H
