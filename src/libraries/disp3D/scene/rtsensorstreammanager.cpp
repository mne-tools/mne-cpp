//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rtsensorstreammanager.cpp
 * @since March 2026
 * @brief Wiring between the real-time sensor data controller, the field mapper and the bound surface colour buffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorstreammanager.h"
#include "sensorfieldmapper.h"
#include "../renderable/brainsurface.h"
#include "../workers/rtsensordatacontroller.h"

#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorStreamManager::RtSensorStreamManager(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

RtSensorStreamManager::~RtSensorStreamManager()
{
    stopStreaming();
}

//=============================================================================================================

bool RtSensorStreamManager::startStreaming(const QString &modality,
                                            const SensorFieldMapper &fieldMapper,
                                            const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces)
{
    if (m_isStreaming) {
        qDebug() << "RtSensorStreamManager: Real-time sensor streaming already active";
        return false;
    }

    // Require loaded evoked data and a built mapping matrix
    if (!fieldMapper.isLoaded() || fieldMapper.evoked().isEmpty()) {
        qWarning() << "RtSensorStreamManager: Cannot start sensor streaming — no evoked data loaded";
        return false;
    }

    // Select the mapping matrix and pick vector based on modality
    std::shared_ptr<Eigen::MatrixXf> mappingMat;
    Eigen::VectorXi pick;
    QString surfaceKey;

    if (modality == QStringLiteral("MEG")) {
        mappingMat = fieldMapper.megMapping();
        pick = fieldMapper.megPick();
        surfaceKey = fieldMapper.megSurfaceKey();
    } else if (modality == QStringLiteral("EEG")) {
        mappingMat = fieldMapper.eegMapping();
        pick = fieldMapper.eegPick();
        surfaceKey = fieldMapper.eegSurfaceKey();
    } else {
        qWarning() << "RtSensorStreamManager: Unknown modality:" << modality;
        return false;
    }

    if (!mappingMat || mappingMat->rows() == 0 || pick.size() == 0) {
        qWarning() << "RtSensorStreamManager: Mapping matrix not built for" << modality
                   << "— call loadSensorField() first";
        return false;
    }

    if (surfaceKey.isEmpty() || !surfaces.contains(surfaceKey)) {
        qWarning() << "RtSensorStreamManager: Target surface not found for" << modality
                   << "streaming (key:" << surfaceKey << ")";
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
    m_controller->setMappingMatrix(mappingMat);

    // Propagate current visualization parameters
    m_controller->setColormapType(fieldMapper.colormap());

    // Compute sampling frequency from evoked data
    double sFreq = 1000.0;
    if (fieldMapper.evoked().times.size() > 1) {
        double dt = fieldMapper.evoked().times(1) - fieldMapper.evoked().times(0);
        if (dt > 0) sFreq = 1.0 / dt;
    }
    m_controller->setSFreq(sFreq);

    // Feed evoked time points as data into the queue
    const int nTimePoints = static_cast<int>(fieldMapper.evoked().times.size());
    qDebug() << "RtSensorStreamManager: Feeding" << nTimePoints << modality
             << "sensor time points into real-time queue";
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

    qDebug() << "RtSensorStreamManager: Real-time sensor streaming started for" << modality;
    return true;
}

//=============================================================================================================

void RtSensorStreamManager::stopStreaming()
{
    if (!m_isStreaming) return;

    if (m_controller)
        m_controller->setStreamingState(false);

    m_isStreaming = false;
    qDebug() << "RtSensorStreamManager: Real-time sensor streaming stopped";
}

//=============================================================================================================

void RtSensorStreamManager::pushData(const Eigen::VectorXf &data)
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

void RtSensorStreamManager::setColormap(const QString &name)
{
    if (m_controller)
        m_controller->setColormapType(name);
}
