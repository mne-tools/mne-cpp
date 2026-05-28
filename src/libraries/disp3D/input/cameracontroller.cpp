//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file cameracontroller.cpp
 * @since 2026
 * @date  March 2026
 * @brief Quaternion-based rotation, exponential zoom and screen-to-world pan math producing the per-viewport CameraResult.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cameracontroller.h"

#include <algorithm>
#include <cmath>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CameraResult CameraController::computeForRotation(const QQuaternion &effectiveRotation,
                                                    float vpZoom,
                                                    const QVector2D &pan,
                                                    bool applyPan,
                                                    float aspectRatio) const
{
    CameraResult result;

    // Projection
    float farPlane = std::max(m_sceneSize * 20.0f, 100.0f);
    result.projection.perspective(45.0f, aspectRatio, m_sceneSize * 0.01f, farPlane);

    // Camera position from rotation + zoom
    float baseDistance = m_sceneSize * 1.5f;
    result.distance    = baseDistance - vpZoom * (m_sceneSize * 0.05f);
    result.cameraPos   = effectiveRotation.rotatedVector(QVector3D(0, 0, result.distance));
    result.upVector    = effectiveRotation.rotatedVector(QVector3D(0, 1, 0));
    result.lookAt      = QVector3D(0, 0, 0);

    // Pan: shift look-at and camera position along the view plane
    if (applyPan && (pan.x() != 0.0f || pan.y() != 0.0f)) {
        const QVector3D right = effectiveRotation.rotatedVector(QVector3D(1, 0, 0)).normalized();
        const QVector3D up    = result.upVector.normalized();
        const QVector3D offset = right * pan.x() + up * pan.y();
        result.lookAt    += offset;
        result.cameraPos += offset;
    }

    // View matrix
    result.view.lookAt(result.cameraPos, result.lookAt, result.upVector);

    // Model matrix: translate to scene center
    result.model.translate(-m_sceneCenter);

    return result;
}

//=============================================================================================================

CameraResult CameraController::computeSingleView(float aspectRatio) const
{
    const QQuaternion preset    = perspectivePresetRotation();
    const QQuaternion effective = m_cameraRotation * preset;

    return computeForRotation(effective, m_zoom, QVector2D(), false, aspectRatio);
}

//=============================================================================================================

CameraResult CameraController::computeMultiView(const SubView &subView, float aspectRatio) const
{
    const int preset = std::clamp(subView.preset, 0, 6);
    const QQuaternion presetOffset = multiViewPresetOffset(preset);

    QQuaternion effectiveRotation;
    if (multiViewPresetIsPerspective(preset)) {
        effectiveRotation = subView.perspectiveRotation * presetOffset;
    } else {
        effectiveRotation = presetOffset;
    }

    const bool isPlanar = !multiViewPresetIsPerspective(preset);

    return computeForRotation(effectiveRotation, subView.zoom, subView.pan,
                              isPlanar, aspectRatio);
}

//=============================================================================================================

void CameraController::applyMouseRotation(const QPoint &delta,
                                           QQuaternion &rotation,
                                           float speed)
{
    const QQuaternion preset = perspectivePresetRotation();
    QQuaternion effective    = rotation * preset;

    const QVector3D upAxis    = effective.rotatedVector(QVector3D(0, 1, 0)).normalized();
    const QVector3D rightAxis = effective.rotatedVector(QVector3D(1, 0, 0)).normalized();

    QQuaternion yaw   = QQuaternion::fromAxisAndAngle(upAxis,    -delta.x() * speed);
    QQuaternion pitch = QQuaternion::fromAxisAndAngle(rightAxis, -delta.y() * speed);

    effective = yaw * pitch * effective;
    rotation  = effective * preset.conjugated();
    rotation.normalize();
}

//=============================================================================================================

void CameraController::applyMousePan(const QPoint &delta,
                                      QVector2D &pan,
                                      float sceneSize)
{
    const float panSpeed = sceneSize * 0.002f;
    pan += QVector2D(-delta.x() * panSpeed, delta.y() * panSpeed);
}
