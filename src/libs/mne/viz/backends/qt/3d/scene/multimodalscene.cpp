//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     multimodalscene.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    MultimodalScene emits the changes of viz::SceneState as Qt signals.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multimodalscene.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultimodalScene::MultimodalScene(QObject* parent)
: QObject(parent)
{
    // The scene state changes on the caller's thread, so the callbacks emit directly
    setLayersChangedCallback([this]() { emit layersChanged(); });
    setTimeSampleChangedCallback([this](int sample) { emit timeSampleChanged(sample); });
    setTimeCursorChangedCallback([this](double seconds) { emit timeCursorChanged(seconds); });
    setOverlayThresholdsChangedCallback([this](float fmin, float fmid, float fmax) { emit overlayThresholdsChanged(fmin, fmid, fmax); });
    setPickedCallback([this](const mne::viz::PickResult& pick) { emit picked(pick); });
}

//=============================================================================================================

MultimodalScene::~MultimodalScene() = default;
