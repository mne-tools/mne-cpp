//=============================================================================================================
/**
 * @file     rtsensorstreammanager.h
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
 * @brief    RtSensorStreamManager class declaration — real-time sensor data streaming.
 *
 */

#ifndef RTSENSORSTREAMMANAGER_H
#define RTSENSORSTREAMMANAGER_H

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
class DISP3DRHISHARED_EXPORT RtSensorStreamManager : public QObject
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
     * @param[in] surfaces    Surface map for target-surface validation.
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
