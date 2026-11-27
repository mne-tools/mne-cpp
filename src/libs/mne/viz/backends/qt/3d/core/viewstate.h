//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viewstate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-viewport state (camera preset, zoom, pan, visibility filter) and serialisation helpers.
 *
 * A mne_viz_qt scene can be displayed either in a single interactive
 * viewport or in a 2x2 multi-view grid. @ref mne::viz::qt::SubView holds the
 * per-pane camera state &mdash; preset (Top / Left / Front /
 * Perspective), zoom, pan offset and rotation quaternion &mdash;
 * and @ref mne::viz::qt::ViewVisibilityProfile holds the per-pane visibility mask
 * (hide skin in the cortex view, hide dipoles in the sensor view,
 * ...).
 *
 * Both structs are POD-like and round-trip through @c QSettings so
 * the user's last camera and per-pane filter are restored when the
 * host application is relaunched.
 */

#ifndef VIEWSTATE_H
#define VIEWSTATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "../../../../brain/rendertypes.h"

#include <mne/viz/views/camera.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QQuaternion>
#include <QVariant>
#include <QColor>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <string_view>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSettings;

namespace mne::viz
{
class BrainSurface;
} // namespace mne::viz

namespace mne::viz::qt
{
//=============================================================================================================
/**
 * @brief Per-view toggle flags controlling which data layers (brain, sensors, sources, network) are visible.
 *
 * Per-object visibility flags for a single viewport.
 *
 * Every viewport (single-view or multi-view pane) carries an independent
 * visibility profile so that each pane can show a different combination of
 * objects.
 */
struct VIZQTSHARED_EXPORT ViewVisibilityProfile
{
    bool lh = true;
    bool rh = true;
    bool bemHead = true;
    bool bemOuterSkull = true;
    bool bemInnerSkull = true;
    bool sensMeg = false;
    bool sensMegGrad = false;
    bool sensMegMag = false;
    bool sensMegHelmet = false;
    bool sensEeg = false;
    bool dig = false;
    bool digCardinal = false;
    bool digHpi = false;
    bool digEeg = false;
    bool digExtra = false;
    bool megFieldMap = false;
    bool eegFieldMap = false;
    bool megFieldContours = false;
    bool eegFieldContours = false;
    bool dipoles = true;
    bool sourceSpace = false;
    bool network = false;
    bool megFieldMapOnHead = false;
    bool mriSlices = true;

    //=========================================================================================================
    /**
     * Query visibility of a named object.
     *
     * @param[in] object   Object key (e.g. "lh", "bem_head", "sens_meg").
     * @return             True if visible.
     */
    bool isObjectVisible(std::string_view object) const;

    //=========================================================================================================
    /**
     * Set visibility of a named object.
     *
     * @param[in] object   Object key.
     * @param[in] visible  New visibility state.
     */
    void setObjectVisible(std::string_view object, bool visible);

    //=========================================================================================================
    /**
     * Load this profile from QSettings under the given key prefix.
     *
     * @param[in] settings Settings store to read from.
     * @param[in] prefix   QSettings key prefix (e.g. "singleVis_").
     */
    void load(const QSettings& settings, const QString& prefix);

    //=========================================================================================================
    /**
     * Save this profile to QSettings under the given key prefix.
     *
     * @param[out] settings  Settings store to write to.
     * @param[in]  prefix    QSettings key prefix.
     */
    void save(QSettings& settings, const QString& prefix) const;
};

//=============================================================================================================
/**
 * @brief Viewport subdivision holding its own camera, projection, and scissor rectangle.
 *
 * Encapsulates all per-view state for a single viewport (single view or
 * one pane in multi-view).
 *
 * The class stores shader, surface type, overlay mode, visibility flags,
 * and, through its toolkit-neutral @ref mne::viz::PaneCamera base, the zoom,
 * pan, camera rotation and view preset — everything that can differ between
 * two viewports showing the same scene.
 */
struct VIZQTSHARED_EXPORT SubView : viz::PaneCamera
{
    // ── Per-view render configuration ──────────────────────────────────
    std::string surfaceType = "pial";
    ShaderMode brainShader = Standard;
    ShaderMode bemShader = Standard;
    VisualizationMode overlayMode = ModeSurface;
    ViewVisibilityProfile visibility;

    // ── Per-view camera state (zoom, pan, rotation, preset in viz::PaneCamera) ──
    bool enabled = true;

    // ── Per-viewport timeline state ───────────────────────────────────
    int currentTimePoint = 0;
    double playbackSpeed = 1.0;
    double stcStepAccum = 0.0;
    bool isPlaying = false;

    // ── FsSurface classification helpers ─────────────────────────────────

    /**
     * True when @p key identifies a brain-tissue surface (lh_/rh_) as opposed
     * to BEM, sensor, digitizer, or source-space geometry.
     *
     * @param[in] key   Surface map key to classify.
     * @return True if the key is non-empty and has no bem_, sens_, srcsp_ or dig_ prefix.
     */
    static bool isBrainSurfaceKey(std::string_view key);

    /**
     * True when the brain-surface @p key matches this view's surfaceType
     * (e.g. "lh_pial" matches surfaceType "pial").
     *
     * @param[in] key   Surface map key to test.
     * @return True if the key is a brain surface key ending with surfaceType.
     */
    bool matchesSurfaceType(std::string_view key) const;

    /**
     * True when the surface identified by @p key should be rendered
     * according to this view's visibility profile.
     *
     * @param[in] key   Surface map key to test.
     * @return True if the visibility profile enables this surface; unknown keys return true.
     */
    bool shouldRenderSurface(std::string_view key) const;

    /**
     * Apply this view's overlayMode to every brain surface whose key
     * matches surfaceType.  Non-brain surfaces are never touched.
     *
     * @param[in, out] surfaces  Surface map keyed by surface name; matching surfaces get their visualization mode updated.
     */
    void applyOverlayToSurfaces(
        std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces) const;

    // ── Serialisation ──────────────────────────────────────────────────

    /**
     * Load this SubView's state from QSettings under the given prefix.
     *
     * @param[in] settings  Settings store to read from.
     * @param[in] prefix    Key prefix (e.g. "multi0_").
     * @param[in] fallbackRotation  Rotation to use if no saved quaternion exists.
     */
    void load(const QSettings& settings, const QString& prefix,
              const Eigen::Quaternionf& fallbackRotation = Eigen::Quaternionf::Identity());

    /**
     * Save this SubView's state to QSettings under the given prefix.
     *
     * @param[out] settings  Settings store to write to.
     * @param[in]  prefix    Key prefix (e.g. "multi0_").
     */
    void save(QSettings& settings, const QString& prefix) const;

    // ── Factory ─────────────────────────────────────────────────────────

    /**
     * Return default SubView configuration for viewport @p index.
     *
     * The preset cycles through the 7 camera orientations (Top, Perspective,
     * Front, Left, Bottom, Back, Right) and the shader cycles through
     * Anatomical → Standard → Holographic.
     *
     * @param[in] index  Zero-based viewport index.
     * @return Enabled SubView with preset index % 7 and the cycled brain shader.
     */
    static SubView defaultForIndex(int index);
};

//=============================================================================================================
// FREE FUNCTIONS — camera presets
//=============================================================================================================

/**
 * Default perspective rotation quaternion used for the "Perspective" camera.
 *
 * @return Rotation built from Euler angles (-45, -40, -130) degrees.
 */
VIZQTSHARED_EXPORT QQuaternion perspectivePresetRotation();

/**
 * Human-readable name for a multi-view preset index (0–6).
 *
 * @param[in] preset  Preset index (0=Top, 1=Perspective, ..., 6=Right).
 * @return Preset name such as "Front"; out-of-range indices return "Top".
 */
VIZQTSHARED_EXPORT std::string multiViewPresetName(int preset);

/**
 * Camera rotation quaternion for a multi-view preset index (0–6).
 *
 * @param[in] preset  Preset index (0=Top, 1=Perspective, ..., 6=Right).
 * @return Camera rotation for the preset; out-of-range indices return the Top rotation.
 */
VIZQTSHARED_EXPORT QQuaternion multiViewPresetOffset(int preset);

/**
 * Whether a preset is a perspective (free-rotate) camera rather than an
 * orthographic-style fixed camera.
 *
 * @param[in] preset  Preset index (0=Top, 1=Perspective, ..., 6=Right).
 * @return True only for preset 1 (Perspective).
 */
VIZQTSHARED_EXPORT bool multiViewPresetIsPerspective(int preset);

//=============================================================================================================
// FREE FUNCTIONS — enum ↔ string conversion
//=============================================================================================================

/**
 * Clamp a visualization target index to [-1, maxIndex].
 *
 * @param[in] target   Raw target index (-1 = single view, 0+ = multi pane).
 * @param[in] maxIndex Upper bound (inclusive).  Defaults to 3 for backwards
 *                     compatibility, but callers should pass viewportCount-1.
 * @return Target index clamped to [-1, maxIndex].
 */
VIZQTSHARED_EXPORT int normalizedVisualizationTarget(int target, int maxIndex = 3);

/**
 * Convert a shader name ("Standard", "Holographic", "Anatomical") to enum.
 *
 * @param[in] name  Shader name; "XRay" is also recognised.
 * @return Matching ShaderMode, or Standard for unknown names.
 */
VIZQTSHARED_EXPORT ShaderMode shaderModeFromName(std::string_view name);

/**
 * Convert a ShaderMode enum to display string.
 *
 * @param[in] mode  Shader mode to convert.
 * @return "Holographic", "Anatomical", or "Standard" for all other modes.
 */
VIZQTSHARED_EXPORT std::string shaderModeName(ShaderMode mode);

/**
 * Convert a visualization mode name to enum.
 *
 * @param[in] name  Mode name ("Annotation", "Scientific", "Source Estimate").
 * @return Matching VisualizationMode, or ModeSurface for unknown names.
 */
VIZQTSHARED_EXPORT VisualizationMode visualizationModeFromName(std::string_view name);

/**
 * Convert a VisualizationMode enum to display string.
 *
 * @param[in] mode  Visualization mode to convert.
 * @return "Annotation", "Scientific", "Source Estimate", or "Surface".
 */
VIZQTSHARED_EXPORT std::string visualizationModeName(VisualizationMode mode);

//=============================================================================================================
// FREE FUNCTIONS — colormap
//=============================================================================================================

/**
 * MNE analyze colormap: teal → blue → gray → red → yellow.
 *
 * Port of mne_analyze_colormap(format='vtk') from MNE-Python.
 * Input v is normalised to [0,1] where 0.5 corresponds to zero field.
 *
 * @param[in] v   Normalised value in [0,1].
 * @return        Packed QRgb colour.
 */
VIZQTSHARED_EXPORT QRgb mneAnalyzeColor(double v);

//=============================================================================================================
// FREE FUNCTIONS — QSettings helpers
//=============================================================================================================

/**
 * Read a boolean from QSettings with a default fallback.
 *
 * @param[in] value     Value read from QSettings.
 * @param[in] fallback  Result to use when the value is invalid (key missing).
 * @return The value converted to bool, or fallback if the value is invalid.
 */
inline bool isTrue(const QVariant& value, bool fallback)
{
    return value.isValid() ? value.toBool() : fallback;
}

} // namespace mne::viz::qt

#endif // VIEWSTATE_H
