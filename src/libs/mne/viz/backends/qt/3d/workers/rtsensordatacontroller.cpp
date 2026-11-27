//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensordatacontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    RtSensorDataController hands the results of viz::RtSensorStream over to its thread as Qt signals.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordatacontroller.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataController::RtSensorDataController(QObject* parent)
: QObject(parent)
{
    // The callbacks run on the streaming and interpolation threads; the signals are emitted on this object's thread
    setNewSensorColorsAvailableCallback([this](const std::string& surfaceKey, const std::vector<uint32_t>& colors) {
        QMetaObject::invokeMethod(this, [this, surfaceKey, colors] { emit newSensorColorsAvailable(surfaceKey, colors); }, Qt::QueuedConnection);
    });
    setNewRawSensorDataAvailableCallback([this](const Eigen::VectorXf& data) {
        QMetaObject::invokeMethod(this, [this, data] { emit newRawSensorDataAvailable(data); }, Qt::QueuedConnection);
    });
    setNewMegMappingAvailableCallback([this](const std::string& surfaceKey, const std::shared_ptr<Eigen::MatrixXf>& mappingMat, const std::vector<int>& pick) {
        QMetaObject::invokeMethod(this, [this, surfaceKey, mappingMat, pick] { emit newMegMappingAvailable(surfaceKey, mappingMat, pick); }, Qt::QueuedConnection);
    });
    setNewEegMappingAvailableCallback([this](const std::string& surfaceKey, const std::shared_ptr<Eigen::MatrixXf>& mappingMat, const std::vector<int>& pick) {
        QMetaObject::invokeMethod(this, [this, surfaceKey, mappingMat, pick] { emit newEegMappingAvailable(surfaceKey, mappingMat, pick); }, Qt::QueuedConnection);
    });
}

//=============================================================================================================

RtSensorDataController::~RtSensorDataController()
{
    // The stream base joins its threads next; results they still hand over are dropped with this object
}
