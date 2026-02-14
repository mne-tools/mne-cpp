//=============================================================================================================
/**
 * @file     cameracontroller.h
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
 * @brief    CameraController class declaration.
 *
 */

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../core/viewstate.h"

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
class CameraController
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
