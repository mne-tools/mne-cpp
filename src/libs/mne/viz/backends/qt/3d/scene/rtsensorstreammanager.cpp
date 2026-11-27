//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorstreammanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Wiring between the real-time sensor data controller, the field mapper and the bound surface colour buffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorstreammanager.h"

#include "sensorfieldmapper.h"
#include "../../../../brain/brainsurface.h"
#include "../workers/rtsensordatacontroller.h"
#include <mne/base/log.h>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorStreamManager::RtSensorStreamManager(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

RtSensorStreamManager::~RtSensorStreamManager()
{
    stopStreaming();
}

//=============================================================================================================

bool RtSensorStreamManager::startStreaming(const std::string& modality,
                                           const SensorFieldMapper& fieldMapper,
                                           const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    if (m_isStreaming) {
        mne::Log::debug("RtSensorStreamManager: Real-time sensor streaming already active");
        return false;
    }

    // Require loaded evoked data and a built mapping matrix
    if (!fieldMapper.isLoaded() || fieldMapper.evoked().empty()) {
        mne::Log::warning("RtSensorStreamManager: Cannot start sensor streaming — no evoked data loaded");
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
        mne::Log::warning("RtSensorStreamManager: Unknown modality: {}", modality);
        return false;
    }

    if (!mappingMat || mappingMat->rows() == 0 || pick.size() == 0) {
        mne::Log::warning("RtSensorStreamManager: Mapping matrix not built for {} — call loadSensorField() first", modality);
        return false;
    }

    if (surfaceKey.empty() || !surfaces.contains(surfaceKey)) {
        mne::Log::warning("RtSensorStreamManager: Target surface not found for {} streaming (key: {})", modality, surfaceKey);
        return false;
    }

    m_modality = modality;

    // Create controller on first use
    if (!m_controller) {
        m_controller = std::make_unique<RtSensorDataController>(this);
        connect(m_controller.get(), &RtSensorDataController::newSensorColorsAvailable,
                this, &RtSensorStreamManager::colorsAvailable);
    }

    // Propagate mapping matrix
    m_controller->setMappingMatrix(surfaceKey, mappingMat);

    // Propagate current visualization parameters
    m_controller->setColormapType(fieldMapper.colormap());

    // Compute sampling frequency from evoked data
    double sFreq = 1000.0;
    if (fieldMapper.evoked().times.size() > 1) {
        double dt = fieldMapper.evoked().times(1) - fieldMapper.evoked().times(0);
        if (dt > 0)
            sFreq = 1.0 / dt;
    }
    m_controller->setSFreq(sFreq);

    // Feed evoked time points as data into the queue
    const int nTimePoints = static_cast<int>(fieldMapper.evoked().times.size());
    mne::Log::debug("RtSensorStreamManager: Feeding {} {} sensor time points into real-time queue", nTimePoints, modality);
    m_controller->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i) {
            meas(i) = static_cast<float>(fieldMapper.evoked().data(pick(i), t));
        }
        m_controller->addData(meas);
    }

    // Start streaming
    m_controller->setStreamingState(true);
    m_isStreaming = true;

    mne::Log::debug("RtSensorStreamManager: Real-time sensor streaming started for {}", modality);
    return true;
}

//=============================================================================================================

void RtSensorStreamManager::stopStreaming()
{
    if (!m_isStreaming)
        return;

    if (m_controller)
        m_controller->setStreamingState(false);

    m_isStreaming = false;
    mne::Log::debug("RtSensorStreamManager: Real-time sensor streaming stopped");
}

//=============================================================================================================

void RtSensorStreamManager::pushData(const Eigen::VectorXf& data)
{
    if (m_controller)
        m_controller->addData(data);
}

//=============================================================================================================

void RtSensorStreamManager::setInterval(int msec)
{
    if (m_controller)
        m_controller->setTimeInterval(msec);
}

//=============================================================================================================

void RtSensorStreamManager::setLooping(bool enabled)
{
    if (m_controller)
        m_controller->setLoopState(enabled);
}

//=============================================================================================================

void RtSensorStreamManager::setAverages(int numAvr)
{
    if (m_controller)
        m_controller->setNumberAverages(numAvr);
}

//=============================================================================================================

void RtSensorStreamManager::setColormap(const std::string& name)
{
    if (m_controller)
        m_controller->setColormapType(name);
}

} // namespace mne::viz::qt
