//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file cameracontroller.h
 * @since 2026
 * @date  March 2026
 * @brief Camera state and projection / view / model matrix computation for single- and multi-view layouts.
 *
 * CameraController is widget-agnostic: it takes the visible scene
 * centroid + extent and a per-pane @ref SubView and produces the
 * @ref CameraResult bundle (projection, view, model, eye position,
 * look-at and up vector) consumed by @ref BrainRenderer.
 *
 * It also encodes the mouse-interaction conventions used everywhere
 * in disp3D: left-drag rotates via quaternion accumulation, wheel
 * zooms by exponential scaling around the scene centre, middle-drag
 * pans by projecting screen-space deltas into the camera's local
 * right / up basis, and the multi-view perspective pane orbits
 * while the orthographic Top / Left / Front panes only pan + zoom.
 */

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include "core/viewstate.h"

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QVector2D>
#include <QPoint>
#include <QSize>

//=============================================================================================================
/**
 * Result of a camera computation — all matrices and vectors needed for
 * rendering a single viewport.
 *
 * @brief Computed camera matrices (projection, view, model) and vectors for a single viewport.
 */
struct CameraResult
{
    QMatrix4x4  projection;
    QMatrix4x4  view;
    QMatrix4x4  model;
    QVector3D   cameraPos;
    QVector3D   upVector;
    QVector3D   lookAt;
    float       distance = 0.0f;
};

//=============================================================================================================
/**
 * Encapsulates camera state and produces view/projection matrices.
 *
 * This class is independent of any Qt widget — it takes scene geometry
 * parameters and view configuration, and produces camera matrices.
 * It also handles rotation, zoom, and pan from mouse delta values.
 *
 * @brief    Camera state and matrix computation.
 */
class DISP3DSHARED_EXPORT CameraController
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     */
    CameraController() = default;

    // ── Scene geometry ─────────────────────────────────────────────────

    /** Set the scene center (centroid of visible objects). */
    void setSceneCenter(const QVector3D &center) { m_sceneCenter = center; }

    /** Set the scene size (extent of visible objects). */
    void setSceneSize(float size)                { m_sceneSize = (size > 0.01f) ? size : 0.3f; }

    QVector3D sceneCenter() const { return m_sceneCenter; }
    float     sceneSize()   const { return m_sceneSize; }

    // ── Single-view camera state ───────────────────────────────────────

    /** Set user rotation for single-view. */
    void setRotation(const QQuaternion &q) { m_cameraRotation = q; }
    QQuaternion rotation() const           { return m_cameraRotation; }

    /** Single-view zoom level. */
    void  setZoom(float z)     { m_zoom = z; }
    float zoom() const         { return m_zoom; }

    /** Reset single-view rotation to identity. */
    void resetRotation()       { m_cameraRotation = QQuaternion(); }

    // ── Matrix computation ─────────────────────────────────────────────

    /**
     * Compute camera matrices for a single-view viewport.
     *
     * @param[in] aspectRatio    Width / height of the viewport.
     * @return                   CameraResult with all matrices.
     */
    CameraResult computeSingleView(float aspectRatio) const;

    /**
     * Compute camera matrices for a multi-view pane.
     *
     * @param[in] subView        Per-view state (preset, zoom, pan, rotation).
     * @param[in] aspectRatio    Width / height of the pane.
     * @return                   CameraResult with all matrices.
     */
    CameraResult computeMultiView(const SubView &subView, float aspectRatio) const;

    // ── Mouse interaction ──────────────────────────────────────────────

    /**
     * Apply rotation from mouse drag delta (single-view or perspective pane).
     *
     * @param[in] delta          Mouse delta in pixels.
     * @param[in,out] rotation   Rotation quaternion to update.
     * @param[in] speed          Rotation speed multiplier (default 0.5).
     */
    static void applyMouseRotation(const QPoint &delta,
                                   QQuaternion &rotation,
                                   float speed = 0.5f);

    /**
     * Apply pan from mouse drag delta (planar multi-view pane).
     *
     * @param[in] delta          Mouse delta in pixels.
     * @param[in,out] pan        Pan offset to update.
     * @param[in] sceneSize      Current scene size for scaling.
     */
    static void applyMousePan(const QPoint &delta,
                              QVector2D &pan,
                              float sceneSize);

private:
    CameraResult computeForRotation(const QQuaternion &effectiveRotation,
                                    float zoom,
                                    const QVector2D &pan,
                                    bool applyPan,
                                    float aspectRatio) const;

    QQuaternion m_cameraRotation;
    QVector3D   m_sceneCenter = QVector3D(0, 0, 0);
    float       m_sceneSize   = 0.3f;
    float       m_zoom        = 0.0f;
};

#endif // CAMERACONTROLLER_H
