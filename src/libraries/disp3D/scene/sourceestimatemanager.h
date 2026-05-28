//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimatemanager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Owns the source-time-course overlay together with its loader, real-time controller and target cortical surfaces.
 *
 * SourceEstimateManager couples @ref SourceEstimateOverlay (the
 * static colour-mapped renderable), @ref StcLoadingWorker (background
 * loader for @c .stc files) and @ref RtSourceDataController (the
 * real-time pipeline) into a single object that the GUI talks to.
 * It exposes time-point scrubbing, playback, looping and threshold /
 * colormap selection without leaking the worker-thread plumbing.
 */

#ifndef SOURCEESTIMATEMANAGER_H
#define SOURCEESTIMATEMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

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
class DISP3DSHARED_EXPORT SourceEstimateManager : public QObject
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
     * @param[in] surfaces           FsSurface map for finding lh/rh brain surfaces.
     * @param[in] activeSurfaceType  Active surface type name (e.g. "pial").
     * @return true if loading was started, false on error.
     */
    bool load(const QString &lhPath, const QString &rhPath,
              const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
              const QString &activeSurfaceType);

    /**
     * Cancel any in-progress STC loading / interpolation and wait for the thread to finish.
     */
    void cancelLoading();

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
     * @param[in] surfaces   FsSurface map.
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
     * @param[in] surfaces   FsSurface map (for colour application).
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
