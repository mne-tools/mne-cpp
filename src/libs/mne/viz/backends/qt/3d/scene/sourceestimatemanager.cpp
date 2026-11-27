//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimatemanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    SourceEstimateManager emits the results of viz::SourceEstimateSession as Qt signals.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimatemanager.h"
#include "../../qtdispatcher.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateManager::SourceEstimateManager(QObject* parent)
: QObject(parent)
, viz::SourceEstimateSession(queuedDispatcher(this))
{
    // The dispatcher delivers on this object's thread, so the callbacks emit directly
    setLoadedCallback([this](int numTimePoints) { emit loaded(numTimePoints); });
    setThresholdsUpdatedCallback([this](float min, float mid, float max) { emit thresholdsUpdated(min, mid, max); });
    setTimePointChangedCallback([this](int index, float time) { emit timePointChanged(index, time); });
    setLoadingProgressCallback([this](int percent, const std::string& message) { emit loadingProgress(percent, message); });
    setRealtimeColorsAvailableCallback([this](const std::vector<uint32_t>& colorsLh, const std::vector<uint32_t>& colorsRh) { emit realtimeColorsAvailable(colorsLh, colorsRh); });
}

//=============================================================================================================

SourceEstimateManager::~SourceEstimateManager() = default;
