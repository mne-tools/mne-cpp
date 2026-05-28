//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multimodalscene.h
 * @since May 2026
 * @brief Host-agnostic controller that owns the ordered scene-layer stack (MRI slice, BEM, cortex, sensors, dipoles, networks, overlays).
 *
 * MultimodalScene composes the disp3D primitives into a single
 * depth-sorted layer stack so the GUI shell does not need to know
 * the per-modality render order. Each @ref SceneLayer carries its
 * renderable, a sort key and a visibility flag; the controller
 * iterates layers in order and issues the corresponding draw calls.
 *
 * It also routes pick events from @ref RayPicker through every
 * layer and returns a uniform @ref PickResult to the host, hiding
 * the per-primitive intersection logic.
 */

#ifndef DISP3DLIB_MULTIMODALSCENE_H
#define DISP3DLIB_MULTIMODALSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"
#include "pickresult.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>

#include <functional>
#include <memory>

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Modality of a layer in the multimodal scene.
 *
 * The scene draws layers in modality order so opaque geometry (cortex, BEM,
 * MRI slices) renders before translucent overlays (source-estimate, sensor,
 * dipole, electrode highlights, network graph).
 */
enum class SceneLayerKind {
    BrainSurface = 0,    /**< Cortical or BEM surface. */
    MriSlice,            /**< MRI orthogonal slice. */
    Sensor,              /**< MEG/EEG sensor positions. */
    Helmet,              /**< MEG helmet shell. */
    Electrode,           /**< sEEG / ECoG electrode array. */
    Dipole,              /**< Dipole renderable. */
    SourceOverlay,       /**< Source-estimate activation overlay. */
    Network,             /**< Connectivity network graph. */
    Custom               /**< Host-app-specific renderable. */
};

//=============================================================================================================
/**
 * @brief Opaque, type-erased handle to a layer payload.
 *
 * The payload is owned by the caller (e.g. the plugin that loaded the data)
 * and supplied as a `std::shared_ptr<void>` so the scene neither owns nor
 * inspects it. The scene only needs the @ref kind, the @ref id, and the
 * generic visibility / opacity flags to drive draw order; the renderer
 * downcasts the payload by @ref kind.
 *
 * This indirection keeps `disp3D/scene/` free of QRhi includes and means
 * new renderables (sEEG, ECoG, future fNIRS optodes, …) plug in without
 * touching this header.
 */
struct DISP3DSHARED_EXPORT SceneLayer
{
    QString                  id;              /**< Caller-supplied unique id ("cortex_lh", "seeg_LH", "mri_axial"). */
    QString                  displayName;     /**< Human-readable label for the scene tree dock. */
    SceneLayerKind           kind = SceneLayerKind::Custom;
    bool                     visible = true;
    float                    opacity = 1.0f;
    int                      drawOrder = 0;   /**< Tie-breaker within the same kind. */
    std::shared_ptr<void>    payload;          /**< Renderable-specific data (ElectrodeObject, SliceObject, BrainSurface, …). */
};

//=============================================================================================================
/**
 * @brief Host-app-agnostic controller that owns the ordered renderable set
 *        and dispatches picks back to the producing layer.
 *
 * Responsibilities:
 *   - Maintain an ordered registry of @ref SceneLayer payloads keyed by id.
 *   - Emit @ref layersChanged whenever a layer is added, removed, or its
 *     visibility/opacity flips, so the renderer can rebuild its draw list.
 *   - Maintain a shared timeline (current time index) that data overlays
 *     across modalities consume; emits @ref timeSampleChanged.
 *   - Maintain the most recent @ref PickResult and emit @ref picked when
 *     a layer producer reports a hit; consumers (Pick dock, status bar,
 *     MRI ortho viewer) subscribe to that signal.
 *
 * The class is @b non-virtual on purpose: it is a pure data/dispatch hub
 * with no rendering logic. The renderer (`BrainRenderer` or a future
 * `MultimodalRenderer`) reads layers via @ref layers() and per-kind
 * downcasts the payload pointer.
 */
class DISP3DSHARED_EXPORT MultimodalScene : public QObject
{
    Q_OBJECT

public:
    explicit MultimodalScene(QObject* parent = nullptr);
    ~MultimodalScene() override;

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
    bool removeLayer(const QString& id);

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
    QVector<SceneLayer> layers() const;

    //=========================================================================================================
    /**
     * @param[in] id  Layer id.
     * @return Pointer to the layer with the given id, or nullptr.
     *         The pointer is valid only until the next mutation.
     */
    const SceneLayer* findLayer(const QString& id) const;

    //=========================================================================================================
    /**
     * Toggle visibility of a layer. No-op if the id is unknown.
     */
    void setLayerVisible(const QString& id, bool visible);

    //=========================================================================================================
    /**
     * Set per-layer opacity in [0, 1]. No-op if the id is unknown.
     */
    void setLayerOpacity(const QString& id, float opacity);

    //=========================================================================================================
    /**
     * @return Current shared time index (used by data overlays). -1 if
     *         no time-resolved data is loaded.
     */
    int currentTimeSample() const;

    //=========================================================================================================
    /**
     * Set the current time index. Emits @ref timeSampleChanged if it
     * actually changes. Negative values are clamped to -1.
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
     * Set the shared time cursor in seconds. Emits @ref timeCursorChanged
     * if the value actually changes.
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
     * are clamped to fmin <= fmid <= fmax. Emits
     * @ref overlayThresholdsChanged if any value actually changes.
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
     * Report a pick result from a layer's renderer or hit-tester. Emits
     * @ref picked. Used by both real ray-cast picking and synthetic picks
     * (e.g. wizard "show this contact" navigation).
     */
    void reportPick(const PickResult& pick);

    //=========================================================================================================
    /**
     * @return Scene-wide axis-aligned bounding box union of all visible
     *         layers, computed by the supplied per-kind extractor. The
     *         scene itself does not know how to read each payload type;
     *         the host registers extractors via @ref registerBoundsFn.
     */
    void worldBounds(QVector3D& bbMin, QVector3D& bbMax) const;

    //=========================================================================================================
    /**
     * Per-kind callback that returns a layer's AABB in world coordinates.
     * The host registers one fn per payload type it actually loads. Layers
     * whose kind has no registered fn are silently skipped during bounds
     * computation.
     */
    using BoundsFn = std::function<bool(const SceneLayer& layer,
                                        QVector3D& bbMin,
                                        QVector3D& bbMax)>;

    //=========================================================================================================
    /**
     * Register an AABB extractor for a given layer kind. Replaces any
     * previously registered fn for that kind.
     */
    void registerBoundsFn(SceneLayerKind kind, BoundsFn fn);

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
    void picked(const DISP3DLIB::PickResult& pick);

private:
    QVector<SceneLayer>                       m_layers;       /**< Insertion-ordered layers. */
    QHash<QString, int>                       m_indexById;     /**< id -> position in m_layers. */
    QHash<int, BoundsFn>                      m_boundsFns;     /**< kind -> AABB extractor. */
    int                                       m_currentTimeSample = -1;
    double                                    m_timeCursor = 0.0;
    float                                     m_overlayFmin = 0.0f;
    float                                     m_overlayFmid = 0.5f;
    float                                     m_overlayFmax = 1.0f;
    PickResult                                m_lastPick;

    /** Re-sort m_layers by (kind, drawOrder, insertion) and refresh m_indexById. */
    void rebuildOrder();
};

} // namespace DISP3DLIB

#endif // DISP3DLIB_MULTIMODALSCENE_H
