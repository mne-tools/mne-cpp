//=============================================================================================================
/**
 * @file     rtsensorstreammanager.cpp
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
 * @brief    RtSensorStreamManager class definition.
 *
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
    QSharedPointer<Eigen::MatrixXf> mappingMat;
    QVector<int> pick;
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

    if (!mappingMat || mappingMat->rows() == 0 || pick.isEmpty()) {
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
            meas(i) = static_cast<float>(fieldMapper.evoked().data(pick[i], t));
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
