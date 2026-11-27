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
 * SourceEstimateManager couples @ref mne::viz::SourceEstimateOverlay (the
 * static colour-mapped renderable), @ref mne::viz::StcLoadingWorker (background
 * loader for @c .stc files) and @ref mne::viz::qt::RtSourceDataController (the
 * real-time pipeline) into a single object that the GUI talks to.
 * It exposes time-point scrubbing, playback, looping and threshold /
 * colormap selection without leaking the worker-thread plumbing.
 */

#ifndef VIZ_QT_SOURCEESTIMATEMANAGER_H
#define VIZ_QT_SOURCEESTIMATEMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/brain/sourceestimatesession.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief Qt face of @ref mne::viz::SourceEstimateSession as a QObject: the same API, with the results as signals on this object's thread.
 */
class VIZQTSHARED_EXPORT SourceEstimateManager : public QObject, public viz::SourceEstimateSession
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit SourceEstimateManager(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~SourceEstimateManager() override;

signals:
    //=========================================================================================================
    /** Emitted when STC data finishes loading. */
    void loaded(int numTimePoints);

    /** Emitted when thresholds are auto-computed from data. */
    void thresholdsUpdated(float min, float mid, float max);

    /** Emitted when the active time point changes. */
    void timePointChanged(int index, float time);

    /** Emitted during async loading to report progress. */
    void loadingProgress(int percent, const std::string& message);

    /**
     * Emitted when the real-time pipeline produces a new set of
     * per-vertex colours.  The host widget should apply these to its
     * surfaces and repaint.
     */
    void realtimeColorsAvailable(const std::vector<uint32_t>& colorsLh,
                                 const std::vector<uint32_t>& colorsRh);
};

} // namespace mne::viz::qt

#endif // VIZ_QT_SOURCEESTIMATEMANAGER_H
