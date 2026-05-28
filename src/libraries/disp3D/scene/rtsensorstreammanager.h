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
 * @ref RtSensorDataController plus the dense MEG / EEG mapping
 * matrix produced by @ref SensorFieldMapper and forwards new
 * colour buffers to the bound @ref BrainSurface on every frame.
 */

#ifndef RTSENSORSTREAMMANAGER_H
#define RTSENSORSTREAMMANAGER_H

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

class BrainSurface;
class RtSensorDataController;
class SensorFieldMapper;

//=============================================================================================================
/**
 * Manages real-time sensor data streaming via RtSensorDataController.
 *
 * BrainView delegates all sensor streaming operations to this component,
 * keeping its own code free of the RtSensorDataController lifecycle.
 *
 * @brief    Real-time sensor streaming lifecycle manager.
 */
class DISP3DSHARED_EXPORT RtSensorStreamManager : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit RtSensorStreamManager(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~RtSensorStreamManager() override;

    // ── Streaming control ──────────────────────────────────────────────

    /**
     * Start real-time sensor streaming for the given modality.
     *
     * Reads mapping matrices, pick vectors, evoked data, and colourmap
     * from the provided @p fieldMapper.  Validates that the target surface
     * key exists in @p surfaces.
     *
     * @param[in] modality    "MEG" or "EEG".
     * @param[in] fieldMapper Sensor-field mapper with built mapping data.
     * @param[in] surfaces    FsSurface map for target-surface validation.
     * @return true if streaming was started successfully.
     */
    bool startStreaming(const QString &modality,
                        const SensorFieldMapper &fieldMapper,
                        const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces);

    /** Stop real-time sensor streaming. */
    void stopStreaming();

    /** @return true while real-time sensor streaming is active. */
    bool isStreaming() const { return m_isStreaming; }

    /** @return the active modality ("MEG" or "EEG"), or empty. */
    QString modality() const { return m_modality; }

    // ── Data input ─────────────────────────────────────────────────────

    /** Push a single measurement vector into the streaming queue. */
    void pushData(const Eigen::VectorXf &data);

    // ── Parameter control ──────────────────────────────────────────────

    /** Set the streaming playback interval in milliseconds. */
    void setInterval(int msec);

    /** Enable or disable looping of the streaming queue. */
    void setLooping(bool enabled);

    /** Set the number of averages for smoothing. */
    void setAverages(int numAvr);

    /** Set the colormap used for colour mapping. */
    void setColormap(const QString &name);

signals:
    //=========================================================================================================
    /**
     * Emitted when the streaming pipeline produces a new set of per-vertex
     * colours for a sensor surface.
     *
     * @param[in] surfaceKey  Key of the target surface.
     * @param[in] colors      Per-vertex ABGR colour array.
     */
    void colorsAvailable(const QString &surfaceKey,
                         const QVector<uint32_t> &colors);

private:
    std::unique_ptr<RtSensorDataController> m_controller;   /**< Real-time sensor data controller. */
    bool m_isStreaming = false;                              /**< True while streaming is active. */
    QString m_modality;                                     /**< Active modality: "MEG" or "EEG". */
};

#endif // RTSENSORSTREAMMANAGER_H
