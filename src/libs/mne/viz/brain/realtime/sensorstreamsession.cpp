//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensorstreamsession.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Wiring between the real-time sensor data controller, the field mapper and the bound surface colour buffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorstreamsession.h"

#include "rtsensorstream.h"
#include "../brainsurface.h"
#include "../sensorfieldmapper.h"
#include <mne/base/log.h>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorStreamSession::SensorStreamSession(Dispatcher dispatcher)
: m_dispatcher(std::move(dispatcher))
{
}

//=============================================================================================================

SensorStreamSession::~SensorStreamSession()
{
    stopStreaming();
}

//=============================================================================================================

bool SensorStreamSession::startStreaming(const std::string& modality,
                                         const SensorFieldMapper& fieldMapper,
                                         const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    if (m_isStreaming) {
        mne::Log::debug("SensorStreamSession: Real-time sensor streaming already active");
        return false;
    }

    // Require loaded evoked data and a built mapping matrix
    if (!fieldMapper.isLoaded() || fieldMapper.evoked().empty()) {
        mne::Log::warning("SensorStreamSession: Cannot start sensor streaming — no evoked data loaded");
        return false;
    }

    // Select the mapping matrix and pick vector based on modality
    std::shared_ptr<Eigen::MatrixXf> mappingMat;
    Eigen::VectorXi pick;
    std::string surfaceKey;

    if (modality == "MEG") {
        mappingMat = fieldMapper.megMapping();
        pick = fieldMapper.megPick();
        surfaceKey = fieldMapper.megSurfaceKey();
    } else if (modality == "EEG") {
        mappingMat = fieldMapper.eegMapping();
        pick = fieldMapper.eegPick();
        surfaceKey = fieldMapper.eegSurfaceKey();
    } else {
        mne::Log::warning("SensorStreamSession: Unknown modality: {}", modality);
        return false;
    }

    if (!mappingMat || mappingMat->rows() == 0 || pick.size() == 0) {
        mne::Log::warning("SensorStreamSession: Mapping matrix not built for {} — call loadSensorField() first", modality);
        return false;
    }

    if (surfaceKey.empty() || !surfaces.contains(surfaceKey)) {
        mne::Log::warning("SensorStreamSession: Target surface not found for {} streaming (key: {})", modality, surfaceKey);
        return false;
    }

    m_modality = modality;

    // Create controller on first use
    if (!m_stream) {
        m_stream = std::make_unique<RtSensorStream>();
        m_stream->setNewSensorColorsAvailableCallback([this](const std::string& key, const std::vector<uint32_t>& colors) {
            m_dispatcher([this, key, colors] {
                if (m_colorsAvailableCallback) {
                    m_colorsAvailableCallback(key, colors);
                }
            });
        });
    }

    // Propagate mapping matrix
    m_stream->setMappingMatrix(surfaceKey, mappingMat);

    // Propagate current visualization parameters
    m_stream->setColormapType(fieldMapper.colormap());

    // Compute sampling frequency from evoked data
    double sFreq = 1000.0;
    if (fieldMapper.evoked().times.size() > 1) {
        double dt = fieldMapper.evoked().times(1) - fieldMapper.evoked().times(0);
        if (dt > 0)
            sFreq = 1.0 / dt;
    }
    m_stream->setSFreq(sFreq);

    // Feed evoked time points as data into the queue
    const int nTimePoints = static_cast<int>(fieldMapper.evoked().times.size());
    mne::Log::debug("SensorStreamSession: Feeding {} {} sensor time points into real-time queue", nTimePoints, modality);
    m_stream->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i) {
            meas(i) = static_cast<float>(fieldMapper.evoked().data(pick(i), t));
        }
        m_stream->addData(meas);
    }

    // Start streaming
    m_stream->setStreamingState(true);
    m_isStreaming = true;

    mne::Log::debug("SensorStreamSession: Real-time sensor streaming started for {}", modality);
    return true;
}

//=============================================================================================================

void SensorStreamSession::stopStreaming()
{
    if (!m_isStreaming)
        return;

    if (m_stream)
        m_stream->setStreamingState(false);

    m_isStreaming = false;
    mne::Log::debug("SensorStreamSession: Real-time sensor streaming stopped");
}

//=============================================================================================================

void SensorStreamSession::pushData(const Eigen::VectorXf& data)
{
    if (m_stream)
        m_stream->addData(data);
}

//=============================================================================================================

void SensorStreamSession::setInterval(int msec)
{
    if (m_stream)
        m_stream->setTimeInterval(msec);
}

//=============================================================================================================

void SensorStreamSession::setLooping(bool enabled)
{
    if (m_stream)
        m_stream->setLoopState(enabled);
}

//=============================================================================================================

void SensorStreamSession::setAverages(int numAvr)
{
    if (m_stream)
        m_stream->setNumberAverages(numAvr);
}

//=============================================================================================================

void SensorStreamSession::setColormap(const std::string& name)
{
    if (m_stream)
        m_stream->setColormapType(name);
}


//=============================================================================================================

void SensorStreamSession::setColorsAvailableCallback(std::function<void(const std::string&, const std::vector<uint32_t>&)> callback)
{
    m_colorsAvailableCallback = std::move(callback);
}

} // namespace mne::viz
