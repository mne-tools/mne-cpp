//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorstreammanager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Manager that wires real-time sensor data into the scene: owns the data controller, the field mapper and the target surface.
 *
 * RtSensorStreamManager bridges the streaming side (sensor packets
 * arriving on a worker thread) and the rendering side (per-vertex
 * colour updates on the target head / helmet surface). It owns the
 * @ref mne::viz::qt::RtSensorDataController plus the dense MEG / EEG mapping
 * matrix produced by @ref mne::viz::SensorFieldMapper and forwards new
 * colour buffers to the bound @ref mne::viz::BrainSurface on every frame.
 */

#ifndef VIZ_QT_RTSENSORSTREAMMANAGER_H
#define VIZ_QT_RTSENSORSTREAMMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/brain/realtime/sensorstreamsession.h>

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
 * @brief Qt face of @ref mne::viz::SensorStreamSession as a QObject: the same API, with the results as signals on this object's thread.
 */
class VIZQTSHARED_EXPORT RtSensorStreamManager : public QObject, public viz::SensorStreamSession
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSensorStreamManager(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~RtSensorStreamManager() override;

signals:
    //=========================================================================================================
    /**
     * Emitted when the streaming pipeline produces a new set of per-vertex
     * colours for a sensor surface.
     *
     * @param[in] surfaceKey  Key of the target surface.
     * @param[in] colors      Per-vertex ABGR colour array.
     */
    void colorsAvailable(const std::string& surfaceKey,
                         const std::vector<uint32_t>& colors);
};

} // namespace mne::viz::qt

#endif // VIZ_QT_RTSENSORSTREAMMANAGER_H
