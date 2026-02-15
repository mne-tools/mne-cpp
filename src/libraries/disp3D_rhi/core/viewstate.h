//=============================================================================================================
/**
 * @file     viewstate.h
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
 * @brief    ViewState declarations — per-view data structures and conversion helpers.
 *
 */

#ifndef VIEWSTATE_H
#define VIEWSTATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include "rendertypes.h"

#include <QString>
#include <QMap>
#include <QQuaternion>
#include <QVector2D>
#include <QVariant>
#include <QColor>

#include <algorithm>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSettings;
class BrainSurface;

//=============================================================================================================
/**
 * Per-object visibility flags for a single viewport.
 *
 * Every viewport (single-view or multi-view pane) carries an independent
 * visibility profile so that each pane can show a different combination of
 * objects.
 */
struct ViewVisibilityProfile
{
    bool lh              = true;
    bool rh              = true;
    bool bemHead         = true;
    bool bemOuterSkull   = true;
    bool bemInnerSkull   = true;
    bool sensMeg         = false;
    bool sensMegGrad     = false;
    bool sensMegMag      = false;
    bool sensMegHelmet   = false;
    bool sensEeg         = false;
    bool dig             = false;
    bool digCardinal     = false;
    bool digHpi          = false;
    bool digEeg          = false;
    bool digExtra        = false;
    bool megFieldMap     = false;
    bool eegFieldMap     = false;
    bool megFieldContours = false;
    bool eegFieldContours = false;
    bool dipoles         = true;
    bool sourceSpace     = false;
    bool network         = false;
    bool megFieldMapOnHead = false;

    //=========================================================================================================
    /**
     * Query visibility of a named object.
     *
     * @param[in] object   Object key (e.g. "lh", "bem_head", "sens_meg").
     * @return             True if visible.
     */
    bool isObjectVisible(const QString &object) const;

    //=========================================================================================================
    /**
     * Set visibility of a named object.
     *
     * @param[in] object   Object key.
     * @param[in] visible  New visibility state.
     */
    void setObjectVisible(const QString &object, bool visible);

    //=========================================================================================================
    /**
     * Load this profile from QSettings under the given key prefix.
     *
     * @param[in] settings Settings store to read from.
     * @param[in] prefix   QSettings key prefix (e.g. "singleVis_").
     */
    void load(const QSettings &settings, const QString &prefix);

    //=========================================================================================================
    /**
     * Save this profile to QSettings under the given key prefix.
     *
     * @param[out] settings  Settings store to write to.
     * @param[in]  prefix    QSettings key prefix.
     */
    void save(QSettings &settings, const QString &prefix) const;
};

//=============================================================================================================
/**
 * Encapsulates all per-view state for a single viewport (single view or
 * one pane in multi-view).
 *
 * The class stores shader, surface type, overlay mode, visibility flags,
 * zoom, pan, camera rotation, and view preset — everything that can differ
 * between two viewports showing the same scene.
 */
struct SubView
{
    // ── Per-view render configuration ──────────────────────────────────
    QString                         surfaceType      = "pial";
    ShaderMode                      brainShader      = Standard;
    ShaderMode                      bemShader        = Standard;
    VisualizationMode               overlayMode      = ModeSurface;
    ViewVisibilityProfile           visibility;

    // ── Per-view camera state ─────────────────────────────────────────
    float                           zoom             = 0.0f;
    QVector2D                       pan;
    QQuaternion                     perspectiveRotation;
    int                             preset           = 1;   // 0=Top,1=Perspective,...,6=Right
    bool                            enabled          = true;

    // ── Surface classification helpers ─────────────────────────────────

    /**
     * True when @p key identifies a brain-tissue surface (lh_/rh_) as opposed
     * to BEM, sensor, digitizer, or source-space geometry.
     */
    static bool isBrainSurfaceKey(const QString &key);

    /**
     * True when the brain-surface @p key matches this view's surfaceType
     * (e.g. "lh_pial" matches surfaceType "pial").
     */
    bool matchesSurfaceType(const QString &key) const;

    /**
     * True when the surface identified by @p key should be rendered
     * according to this view's visibility profile.
     */
    bool shouldRenderSurface(const QString &key) const;

    /**
     * Apply this view's overlayMode to every brain surface whose key
     * matches surfaceType.  Non-brain surfaces are never touched.
     */
    void applyOverlayToSurfaces(
        QMap<QString, std::shared_ptr<BrainSurface>> &surfaces) const;

    // ── Serialisation ──────────────────────────────────────────────────

    /**
     * Load this SubView's state from QSettings under the given prefix.
     *
     * @param[in] settings  Settings store to read from.
     * @param[in] prefix    Key prefix (e.g. "multi0_").
     * @param[in] fallbackRotation  Rotation to use if no saved quaternion exists.
     */
    void load(const QSettings &settings, const QString &prefix,
              const QQuaternion &fallbackRotation = QQuaternion());

    /**
     * Save this SubView's state to QSettings under the given prefix.
     *
     * @param[out] settings  Settings store to write to.
     * @param[in]  prefix    Key prefix (e.g. "multi0_").
     */
    void save(QSettings &settings, const QString &prefix) const;

    // ── Factory ─────────────────────────────────────────────────────────

    /**
     * Return default SubView configuration for viewport @p index.
     *
     * The preset cycles through the 7 camera orientations (Top, Perspective,
     * Front, Left, Bottom, Back, Right) and the shader cycles through
     * Anatomical → Standard → Holographic.
     */
    static SubView defaultForIndex(int index);
};

//=============================================================================================================
// FREE FUNCTIONS — camera presets
//=============================================================================================================

/**
 * Default perspective rotation quaternion used for the "Perspective" camera.
 */
QQuaternion perspectivePresetRotation();

/**
 * Human-readable name for a multi-view preset index (0–6).
 */
QString multiViewPresetName(int preset);

/**
 * Camera rotation quaternion for a multi-view preset index (0–6).
 */
QQuaternion multiViewPresetOffset(int preset);

/**
 * Whether a preset is a perspective (free-rotate) camera rather than an
 * orthographic-style fixed camera.
 */
bool multiViewPresetIsPerspective(int preset);

//=============================================================================================================
// FREE FUNCTIONS — enum ↔ string conversion
//=============================================================================================================

/**
 * Clamp a visualization target index to [-1, maxIndex].
 *
 * @param[in] target   Raw target index (-1 = single view, 0+ = multi pane).
 * @param[in] maxIndex Upper bound (inclusive).  Defaults to 3 for backwards
 *                     compatibility, but callers should pass viewportCount-1.
 */
int normalizedVisualizationTarget(int target, int maxIndex = 3);

/** Convert a shader name ("Standard", "Holographic", "Anatomical") to enum. */
ShaderMode shaderModeFromName(const QString &name);

/** Convert a ShaderMode enum to display string. */
QString shaderModeName(ShaderMode mode);

/** Convert a visualization mode name to enum. */
VisualizationMode visualizationModeFromName(const QString &name);

/** Convert a VisualizationMode enum to display string. */
QString visualizationModeName(VisualizationMode mode);

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
QRgb mneAnalyzeColor(double v);

//=============================================================================================================
// FREE FUNCTIONS — QSettings helpers
//=============================================================================================================

/**
 * Read a boolean from QSettings with a default fallback.
 */
inline bool isTrue(const QVariant &value, bool fallback)
{
    return value.isValid() ? value.toBool() : fallback;
}

#endif // VIEWSTATE_H
