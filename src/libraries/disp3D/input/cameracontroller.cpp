//=============================================================================================================
/**
 * @file     cameracontroller.cpp
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
 * @brief    CameraController class implementation.
 *
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
