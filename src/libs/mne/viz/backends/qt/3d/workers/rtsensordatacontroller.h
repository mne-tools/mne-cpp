//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensordatacontroller.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Real-time MEG / EEG sensor streaming controller that orchestrates the data and field-mapping workers.
 *
 * RtSensorDataController spins up a background @ref
 * mne::viz::RtSensorDataWorker (queue, averaging, dense mat-vec mapping,
 * ABGR packing) and an optional @ref mne::viz::RtSensorInterpolationMatWorker
 * (asynchronous recomputation of the dense mapping matrix when the
 * sensor configuration, projectors or target surface change) and
 * ties them together with a timer that paces the streaming.
 *
 * Once started, the controller emits ready-to-upload per-vertex
 * colour buffers; the GUI just connects
 * @c newSensorColorsAvailable to its surface-colour slot.
 */

#ifndef VIZ_QT_RTSENSORDATACONTROLLER_H
#define VIZ_QT_RTSENSORDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/brain/realtime/rtsensorstream.h>

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
 * @brief Qt face of @ref mne::viz::RtSensorStream as a QObject: the same API, with the results as signals on this object's thread.
 *
 * The streaming and interpolation threads of the stream hand their results over to this object's thread, so
 * slots connected to the signals may touch the GUI.
 */
class VIZQTSHARED_EXPORT RtSensorDataController : public QObject, public viz::RtSensorStream
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSensorDataController(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Stops the threads of the stream before the signals go away.
     */
    ~RtSensorDataController() override;

signals:
    //=========================================================================================================
    /**
     * Emitted when new per-vertex color data for a sensor surface is available.
     *
     * @param[in] surfaceKey  Key identifying the target surface.
     * @param[in] colors      Per-vertex ABGR color array.
     */
    void newSensorColorsAvailable(const std::string& surfaceKey,
                                  const std::vector<uint32_t>& colors);

    //=========================================================================================================
    /**
     * Emitted when raw (non-mapped) sensor data is available.
     *
     * @param[in] data        Raw sensor measurement vector.
     */
    void newRawSensorDataAvailable(const Eigen::VectorXf& data);

    //=========================================================================================================
    /**
     * Emitted when a new MEG mapping matrix has been computed by the background worker.
     *
     * @param[in] surfaceKey    Target surface key.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newMegMappingAvailable(const std::string& surfaceKey,
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                const std::vector<int>& pick);

    //=========================================================================================================
    /**
     * Emitted when a new EEG mapping matrix has been computed by the background worker.
     *
     * @param[in] surfaceKey    Target surface key.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newEegMappingAvailable(const std::string& surfaceKey,
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                const std::vector<int>& pick);
};

} // namespace mne::viz::qt

#endif // VIZ_QT_RTSENSORDATACONTROLLER_H
