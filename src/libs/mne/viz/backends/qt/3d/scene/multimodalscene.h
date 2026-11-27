//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     multimodalscene.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Host-agnostic controller that owns the ordered scene-layer stack (MRI slice, BEM, cortex, sensors, dipoles, networks, overlays).
 *
 * MultimodalScene composes the mne_viz_qt primitives into a single
 * depth-sorted layer stack so the GUI shell does not need to know
 * the per-modality render order. Each @ref mne::viz::SceneLayer carries its
 * renderable, a sort key and a visibility flag; the controller
 * iterates layers in order and issues the corresponding draw calls.
 *
 * It also routes pick events from @ref mne::viz::qt::RayPicker through every
 * layer and returns a uniform @ref mne::viz::PickResult to the host, hiding
 * the per-primitive intersection logic.
 */

#ifndef VIZ_QT_MULTIMODALSCENE_H
#define VIZ_QT_MULTIMODALSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/brain/scenestate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief Qt face of @ref mne::viz::SceneState as a QObject: the same API, with its changes as signals.
 *
 * @snippet ex_disp3d_scene/main.cpp multimodal_scene_usage
 */
class VIZQTSHARED_EXPORT MultimodalScene : public QObject, public viz::SceneState
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit MultimodalScene(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MultimodalScene() override;

signals:
    /**
     * Emitted whenever the layer set or any layer flag (visibility,
     * opacity, drawOrder) changes. The renderer should treat this as
     * "rebuild the draw list".
     */
    void layersChanged();

    /**
     * Emitted when the shared time index changes. Data overlays and
     * timeline scrubbers subscribe.
     */
    void timeSampleChanged(int sample);

    /**
     * Emitted when the shared time cursor (seconds) changes. Driven by
     * the Overlay dock's continuous time slider; consumed by per-sample
     * value lookup on electrodes / surface overlays.
     */
    void timeCursorChanged(double seconds);

    /**
     * Emitted when any of the shared overlay thresholds change.
     */
    void overlayThresholdsChanged(float fmin, float fmid, float fmax);

    /**
     * Emitted when a pick is reported. Pick dock, status bar, and MRI
     * ortho viewer subscribe.
     */
    void picked(const mne::viz::PickResult& pick);
};

} // namespace mne::viz::qt

#endif // VIZ_QT_MULTIMODALSCENE_H
