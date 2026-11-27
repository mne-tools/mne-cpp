//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensorstreamsession.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Streams the evoked data of a sensor field map onto its helmet or head surface.
 *
 * SensorStreamSession bridges the streaming side (sensor packets
 * arriving on a worker thread) and the rendering side (per-vertex
 * colour updates on the target head / helmet surface). It owns the
 * @ref mne::viz::RtSensorStream plus the dense MEG / EEG mapping
 * matrix produced by @ref mne::viz::SensorFieldMapper and forwards new
 * colour buffers to the bound @ref mne::viz::BrainSurface on every frame.
 */

#ifndef VIZ_SENSORSTREAMSESSION_H
#define VIZ_SENSORSTREAMSESSION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"
#include "../../host/dispatcher.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <memory>
#include <string>
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
class SensorFieldMapper;
} // namespace mne::viz

namespace mne::viz
{
class RtSensorStream;
//=============================================================================================================
/**
 * Manages real-time sensor data streaming via RtSensorStream.
 *
 * BrainView delegates all sensor streaming operations to this component,
 * keeping its own code free of the RtSensorStream lifecycle. The colours reach
 * the owner through its dispatcher, on the owner's thread.
 *
 * @brief    Real-time sensor streaming lifecycle manager.
 */
class VIZSHARED_EXPORT SensorStreamSession
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] dispatcher   Hands the colours from the streaming thread to the owner's thread.
     */
    explicit SensorStreamSession(Dispatcher dispatcher = immediate_dispatcher());

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~SensorStreamSession();

    SensorStreamSession(const SensorStreamSession&) = delete;
    SensorStreamSession& operator=(const SensorStreamSession&) = delete;

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
    bool startStreaming(const std::string& modality,
                        const SensorFieldMapper& fieldMapper,
                        const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces);

    /** Stop real-time sensor streaming. */
    void stopStreaming();

    /** @return true while real-time sensor streaming is active. */
    bool isStreaming() const
    {
        return m_isStreaming;
    }

    /** @return the active modality ("MEG" or "EEG"), or empty. */
    std::string modality() const
    {
        return m_modality;
    }

    // ── Data input ─────────────────────────────────────────────────────

    /**
     * Push a single measurement vector into the streaming queue.
     *
     * @param[in] data   One sample of sensor values; ignored if no controller exists.
     */
    void pushData(const Eigen::VectorXf& data);

    // ── Parameter control ──────────────────────────────────────────────

    /**
     * Set the streaming playback interval in milliseconds.
     *
     * @param[in] msec   Interval between streamed samples in milliseconds.
     */
    void setInterval(int msec);

    /**
     * Enable or disable looping of the streaming queue.
     *
     * @param[in] enabled   True to replay the queue from the start when it ends.
     */
    void setLooping(bool enabled);

    /**
     * Set the number of averages for smoothing.
     *
     * @param[in] numAvr   Number of consecutive samples averaged per frame.
     */
    void setAverages(int numAvr);

    /**
     * Set the colormap used for colour mapping.
     *
     * @param[in] name   Colormap name (e.g. "MNE", "Jet").
     */
    void setColormap(const std::string& name);

public:
    //=========================================================================================================
    /**
     * Set the callback for colorsAvailable. Called when the streaming pipeline produces a new set of per-vertex colours for a sensor surface. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, colors); empty to drop the results.
     */
    void setColorsAvailableCallback(std::function<void(const std::string&, const std::vector<uint32_t>&)> callback);

private:
    std::unique_ptr<RtSensorStream> m_stream; /**< Real-time sensor stream. */
    Dispatcher m_dispatcher;                  /**< Hands results to the owner's thread. */
    bool m_isStreaming = false;               /**< True while streaming is active. */
    std::string m_modality;                   /**< Active modality: "MEG" or "EEG". */

    std::function<void(const std::string&, const std::vector<uint32_t>&)> m_colorsAvailableCallback; /**< Receives colorsAvailable. */
};

} // namespace mne::viz

#endif // VIZ_SENSORSTREAMSESSION_H
