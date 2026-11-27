//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     cameracontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    CameraController definition: converts the matrices and drags of viz::OrbitCamera to Qt types.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cameracontroller.h"

#include "../../qtconversions.h"

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

CameraResult toCameraResult(const viz::CameraMatrices& matrices)
{
    CameraResult result;
    result.projection = toQMatrix(matrices.projection);
    result.view = toQMatrix(matrices.view);
    result.model = toQMatrix(matrices.model);
    result.cameraPos = toQVector3D(matrices.eye);
    result.upVector = toQVector3D(matrices.up);
    result.lookAt = toQVector3D(matrices.look_at);
    result.distance = matrices.distance;
    return result;
}

} // namespace

//=============================================================================================================

CameraResult CameraController::computeSingleView(float aspectRatio) const
{
    return toCameraResult(m_camera.single_view(toEigen(m_cameraRotation), m_zoom, aspectRatio));
}

//=============================================================================================================

CameraResult CameraController::computeMultiView(const SubView& subView, float aspectRatio) const
{
    return toCameraResult(m_camera.pane_view(subView, aspectRatio));
}

//=============================================================================================================

void CameraController::applyMouseRotation(const QPoint& delta,
                                          QQuaternion& rotation,
                                          float speed)
{
    rotation = toQQuaternion(viz::OrbitCamera::orbit(toEigen(rotation), delta.x(), delta.y(), speed));
}

//=============================================================================================================

void CameraController::applyMousePan(const QPoint& delta,
                                     QVector2D& pan,
                                     float sceneSize)
{
    pan = toQVector2D(viz::OrbitCamera::panned(toEigen(pan), delta.x(), delta.y(), sceneSize));
}

} // namespace mne::viz::qt
