//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scenestate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Toolkit-free state of a multimodal scene: the ordered scene-layer stack (MRI slice, BEM, cortex, sensors, dipoles, networks, overlays).
 *
 * SceneState composes the brain view primitives into a single
 * depth-sorted layer stack so the GUI shell does not need to know
 * the per-modality render order. Each @ref mne::viz::SceneLayer carries its
 * renderable, a sort key and a visibility flag; the controller
 * iterates layers in order and issues the corresponding draw calls.
 *
 * It also routes pick events from @ref mne::viz::qt::RayPicker through every
 * layer and returns a uniform @ref mne::viz::PickResult to the host, hiding
 * the per-primitive intersection logic.
 */

#ifndef VIZ_SCENESTATE_H
#define VIZ_SCENESTATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "pickresult.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Modality of a layer in the multimodal scene.
 *
 * The scene draws layers in modality order so opaque geometry (cortex, BEM,
 * MRI slices) renders before translucent overlays (source-estimate, sensor,
 * dipole, electrode highlights, network graph).
 */
enum class SceneLayerKind
{
    BrainSurface = 0, /**< Cortical or BEM surface. */
    MriSlice,         /**< MRI orthogonal slice. */
    Sensor,           /**< MEG/EEG sensor positions. */
    Helmet,           /**< MEG helmet shell. */
    Electrode,        /**< sEEG / ECoG electrode array. */
    Dipole,           /**< Dipole renderable. */
    SourceOverlay,    /**< Source-estimate activation overlay. */
    Network,          /**< Connectivity network graph. */
    Custom            /**< Host-app-specific renderable. */
};

//=============================================================================================================
/**
 * @brief Opaque, type-erased handle to a layer payload.
 *
 * The payload is owned by the caller (e.g. the plugin that loaded the data)
 * and supplied as a `std::shared_ptr<void>` so the scene neither owns nor
 * inspects it. The scene only needs the @c mne::viz::SceneLayer::kind "kind", the @ref id, and the
 * generic visibility / opacity flags to drive draw order; the renderer
 * downcasts the payload by @c mne::viz::SceneLayer::kind "kind".
 *
 * This indirection keeps `3d/scene/` free of QRhi includes and means
 * new renderables (sEEG, ECoG, future fNIRS optodes, …) plug in without
 * touching this header.
 *
 * @snippet ex_disp3d_scene/main.cpp multimodal_scene_usage
 */
struct VIZSHARED_EXPORT SceneLayer
{
    std::string id;          /**< Caller-supplied unique id ("cortex_lh", "seeg_LH", "mri_axial"). */
    std::string displayName; /**< Human-readable label for the scene tree dock. */
    SceneLayerKind kind = SceneLayerKind::Custom;
    bool visible = true;
    float opacity = 1.0f;
    int drawOrder = 0;             /**< Tie-breaker within the same kind. */
    std::shared_ptr<void> payload; /**< Renderable-specific data (ElectrodeObject, SliceObject, BrainSurface, …). */
};

//=============================================================================================================
/**
 * @brief Host-app-agnostic controller that owns the ordered renderable set
 *        and dispatches picks back to the producing layer.
 *
 * Responsibilities:
 *   - Maintain an ordered registry of @ref SceneLayer payloads keyed by id.
 *   - Report to setLayersChangedCallback() whenever a layer is added, removed, or its
 *     visibility/opacity flips, so the renderer can rebuild its draw list.
 *   - Maintain a shared timeline (current time index) that data overlays
 *     across modalities consume; reports changes to setTimeSampleChangedCallback().
 *   - Maintain the most recent @ref mne::viz::PickResult and report it to
 *     setPickedCallback() when
 *     a layer producer reports a hit; consumers (Pick dock, status bar,
 *     MRI ortho viewer) subscribe to that signal.
 *
 * It is a pure data/dispatch hub with no rendering logic; qt::MultimodalScene
 * adds Qt signals on top. The renderer (`BrainRenderer` or a future
 * `MultimodalRenderer`) reads layers via @ref layers() and per-kind
 * downcasts the payload pointer.
 *
 * @snippet ex_disp3d_scene/main.cpp multimodal_scene_usage
 */
class VIZSHARED_EXPORT SceneState
{
public:
    SceneState();
    virtual ~SceneState();

    //=========================================================================================================
    /**
     * Add or replace a layer.
     *
     * If a layer with the same @ref SceneLayer::id already exists, the
     * stored layer is overwritten in place (preserving its slot in the
     * draw order). Otherwise the new layer is appended to its kind's
     * group.
     *
     * @param[in] layer  Layer record. The caller retains ownership of the
     *                   underlying payload via the shared_ptr.
     */
    void addLayer(SceneLayer layer);

    //=========================================================================================================
    /**
     * Remove a layer by id.
     *
     * @param[in] id  Layer id supplied to @ref addLayer.
     * @return true if a layer was removed, false if the id was unknown.
     */
    bool removeLayer(const std::string& id);

    //=========================================================================================================
    /**
     * Remove every layer.
     */
    void clear();

    //=========================================================================================================
    /**
     * @return All layers in current draw order (sorted by kind, then by
     *         drawOrder, then by insertion order).
     */
    std::vector<SceneLayer> layers() const;

    //=========================================================================================================
    /**
     * @param[in] id  Layer id.
     * @return Pointer to the layer with the given id, or nullptr.
     *         The pointer is valid only until the next mutation.
     */
    const SceneLayer* findLayer(const std::string& id) const;

    //=========================================================================================================
    /**
     * Toggle visibility of a layer. No-op if the id is unknown.
     *
     * @param[in] id        Layer id.
     * @param[in] visible   True to show the layer, false to hide it.
     */
    void setLayerVisible(const std::string& id, bool visible);

    //=========================================================================================================
    /**
     * Set per-layer opacity in [0, 1]. No-op if the id is unknown.
     *
     * @param[in] id        Layer id.
     * @param[in] opacity   Opacity, clamped to [0, 1].
     */
    void setLayerOpacity(const std::string& id, float opacity);

    //=========================================================================================================
    /**
     * @return Current shared time index (used by data overlays). -1 if
     *         no time-resolved data is loaded.
     */
    int currentTimeSample() const;

    //=========================================================================================================
    /**
     * Set the current time index. Reports to setTimeSampleChangedCallback() if it
     * actually changes. Negative values are clamped to -1.
     *
     * @param[in] sample   New time sample index, or -1 for no time-resolved data.
     */
    void setCurrentTimeSample(int sample);

    //=========================================================================================================
    /**
     * @return Current shared time cursor in seconds. Independent of the
     *         integer-sample timeline (@ref currentTimeSample) — overlay
     *         widgets that drive a continuous time slider operate in
     *         seconds. Default 0.
     */
    double timeCursor() const;

    //=========================================================================================================
    /**
     * Set the shared time cursor in seconds. Reports to setTimeCursorChangedCallback()
     * if the value actually changes.
     *
     * @param[in] seconds   New time cursor position in seconds.
     */
    void setTimeCursor(double seconds);

    //=========================================================================================================
    /**
     * @return Current overlay min threshold. Default 0.
     */
    float overlayFmin() const;

    //=========================================================================================================
    /**
     * @return Current overlay mid threshold. Default 0.5.
     */
    float overlayFmid() const;

    //=========================================================================================================
    /**
     * @return Current overlay max threshold. Default 1.
     */
    float overlayFmax() const;

    //=========================================================================================================
    /**
     * Set the shared (fmin, fmid, fmax) overlay thresholds used by the
     * data-driven Overlay dock and the renderables it drives. The values
     * are clamped to fmin <= fmid <= fmax. Reports to
     * setOverlayThresholdsChangedCallback() if any value actually changes.
     *
     * @param[in] fmin   Minimum threshold.
     * @param[in] fmid   Mid threshold, raised to fmin if smaller.
     * @param[in] fmax   Maximum threshold, raised to fmid if smaller.
     */
    void setOverlayThresholds(float fmin, float fmid, float fmax);

    //=========================================================================================================
    /**
     * @return Most recent pick reported via @ref reportPick. Default-
     *         constructed (kind == None) until the first hit.
     */
    const PickResult& lastPick() const;

    //=========================================================================================================
    /**
     * Report a pick result from a layer's renderer or hit-tester. Reports it to
     * setPickedCallback(). Used by both real ray-cast picking and synthetic picks
     * (e.g. wizard "show this contact" navigation).
     *
     * @param[in] pick   Pick result to store as the last pick and broadcast.
     */
    void reportPick(const PickResult& pick);

    //=========================================================================================================
    /**
     * Scene-wide axis-aligned bounding box union of all visible
     * layers, computed by the supplied per-kind extractor. The
     *         scene itself does not know how to read each payload type;
     *         the host registers extractors via @ref registerBoundsFn.
     *
     * @param[out] bbMin   Minimum corner of the union box; (-1, -1, -1) if no layer contributed.
     * @param[out] bbMax   Maximum corner of the union box; (1, 1, 1) if no layer contributed.
     */
    void worldBounds(Eigen::Vector3f& bbMin, Eigen::Vector3f& bbMax) const;

    //=========================================================================================================
    /**
     * Per-kind callback that returns a layer's AABB in world coordinates.
     * The host registers one fn per payload type it actually loads. Layers
     * whose kind has no registered fn are silently skipped during bounds
     * computation.
     */
    using BoundsFn = std::function<bool(const SceneLayer& layer,
                                        Eigen::Vector3f& bbMin,
                                        Eigen::Vector3f& bbMax)>;

    //=========================================================================================================
    /**
     * Register an AABB extractor for a given layer kind. Replaces any
     * previously registered fn for that kind.
     *
     * @param[in] kind   Layer kind the extractor handles.
     * @param[in] fn     Extractor returning false when the layer has no bounds.
     */
    void registerBoundsFn(SceneLayerKind kind, BoundsFn fn);

public:
    //=========================================================================================================
    /**
     * Set the callback for layersChanged. Called whenever the layer set or any layer flag (visibility, opacity, drawOrder) changes. The renderer should treat this as "rebuild the draw list". It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (nothing); empty to drop the results.
     */
    void setLayersChangedCallback(std::function<void()> callback);

    //=========================================================================================================
    /**
     * Set the callback for timeSampleChanged. Called when the shared time index changes. Data overlays and timeline scrubbers subscribe. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (sample); empty to drop the results.
     */
    void setTimeSampleChangedCallback(std::function<void(int)> callback);

    //=========================================================================================================
    /**
     * Set the callback for timeCursorChanged. Called when the shared time cursor (seconds) changes. Driven by the Overlay dock's continuous time slider; consumed by per-sample value lookup on electrodes / surface overlays. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (seconds); empty to drop the results.
     */
    void setTimeCursorChangedCallback(std::function<void(double)> callback);

    //=========================================================================================================
    /**
     * Set the callback for overlayThresholdsChanged. Called when any of the shared overlay thresholds change. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (fmin, fmid, fmax); empty to drop the results.
     */
    void setOverlayThresholdsChangedCallback(std::function<void(float, float, float)> callback);

    //=========================================================================================================
    /**
     * Set the callback for picked. Called when a pick is reported. Pick dock, status bar, and MRI ortho viewer subscribe. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (pick); empty to drop the results.
     */
    void setPickedCallback(std::function<void(const mne::viz::PickResult&)> callback);

private:
    std::vector<SceneLayer> m_layers;                         /**< Insertion-ordered layers. */
    std::unordered_map<std::string, std::size_t> m_indexById; /**< id -> position in m_layers. */
    std::unordered_map<int, BoundsFn> m_boundsFns;            /**< kind -> AABB extractor. */
    int m_currentTimeSample = -1;
    double m_timeCursor = 0.0;
    float m_overlayFmin = 0.0f;
    float m_overlayFmid = 0.5f;
    float m_overlayFmax = 1.0f;
    PickResult m_lastPick;

    /** Re-sort m_layers by (kind, drawOrder, insertion) and refresh m_indexById. */
    void rebuildOrder();

    std::function<void()> m_layersChangedCallback;                               /**< Receives layersChanged. */
    std::function<void(int)> m_timeSampleChangedCallback;                        /**< Receives timeSampleChanged. */
    std::function<void(double)> m_timeCursorChangedCallback;                     /**< Receives timeCursorChanged. */
    std::function<void(float, float, float)> m_overlayThresholdsChangedCallback; /**< Receives overlayThresholdsChanged. */
    std::function<void(const mne::viz::PickResult&)> m_pickedCallback;           /**< Receives picked. */
};

} // namespace mne::viz

#endif // VIZ_SCENESTATE_H
