//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorstreammanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    RtSensorStreamManager emits the results of viz::SensorStreamSession as Qt signals.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorstreammanager.h"
#include "../../qtdispatcher.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorStreamManager::RtSensorStreamManager(QObject* parent)
: QObject(parent)
, viz::SensorStreamSession(queuedDispatcher(this))
{
    // The dispatcher delivers on this object's thread, so the callbacks emit directly
    setColorsAvailableCallback([this](const std::string& surfaceKey, const std::vector<uint32_t>& colors) { emit colorsAvailable(surfaceKey, colors); });
}

//=============================================================================================================

RtSensorStreamManager::~RtSensorStreamManager() = default;
