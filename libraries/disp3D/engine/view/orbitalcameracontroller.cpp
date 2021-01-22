//=============================================================================================================
/**
 * @file     orbitalcameracontroller.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lars Debor. All rights reserved.
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
 * @brief    OrbitalCameraController class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "orbitalcameracontroller.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender/QCamera>
#include <Qt3DCore/qtransform.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

OrbitalCameraController::OrbitalCameraController(Qt3DCore::QNode *pParent)
    :QAbstractCameraController(pParent)
    , m_target(nullptr)
    , m_matrix()
    , m_radius(1.0f)
    , m_angle(0.0f)
{
    initController();
}

//=============================================================================================================

void OrbitalCameraController::invertCameraRotation(bool newStatusFlag)
{
    if(newStatusFlag == true) {
        m_rotationInversFactor = -1.0f;
    }
    else {
        m_rotationInversFactor = 1.0f;
    }
}

//=============================================================================================================

void OrbitalCameraController::moveCamera(const Qt3DExtras::QAbstractCameraController::InputState &state, float dt)
{
    Qt3DRender::QCamera *pCamera = this->camera();

    if(pCamera == nullptr) {
        return;
    }

    //Mouse input
    if(state.rightMouseButtonActive) {
        if(state.altKeyActive) {
            //translate camera in x/y direction
            pCamera->translate(QVector3D(state.rxAxisValue * this->linearSpeed() * dt * 0.2f,
                                         state.ryAxisValue * this->linearSpeed() * dt * 0.2f,
                                         0.0f));
        }
        else {
            // orbit around view center
            pCamera->panAboutViewCenter(state.rxAxisValue * this->lookSpeed() * dt * m_rotationInversFactor,
                                        QVector3D(0.0f, 0.0f, 1.0f));
            pCamera->tiltAboutViewCenter(state.ryAxisValue * this->lookSpeed() * dt * m_rotationInversFactor);
        }
    }

    if(state.middleMouseButtonActive) {
        //translate the cameras view center
        pCamera->translate(QVector3D(state.rxAxisValue * this->linearSpeed() * dt * 0.2f,
                                     state.ryAxisValue * this->linearSpeed() * dt * 0.2f,
                                     0.0f));
    }

    //zoom with mouse wheel and page up and down
    if(distance(pCamera->position(), pCamera->viewCenter()) > m_fZoomInLimit) {
        pCamera->translate(QVector3D(0.0f, 0.0f, state.tzAxisValue * this->linearSpeed() * dt),
                           pCamera->DontTranslateViewCenter);
    }
    else {
        pCamera->translate(QVector3D(0.0f, 0.0f, -m_fZoomInLimit), pCamera->DontTranslateViewCenter);
    }

    //Keyboard input: orbit around view center
    pCamera->panAboutViewCenter(state.txAxisValue * this->lookSpeed() * dt * 0.8f  * m_rotationInversFactor,
                                QVector3D(0.0f, 0.0f, 1.0f));
    pCamera->tiltAboutViewCenter(state.tyAxisValue * this->lookSpeed()* dt * 0.8f * m_rotationInversFactor);
}

//=============================================================================================================

void OrbitalCameraController::initController()
{
    this->setLinearSpeed(0.55f);
    this->setLookSpeed(143.f);
    invertCameraRotation(true);
}

//=============================================================================================================

void OrbitalCameraController::setTarget(Qt3DCore::QTransform *target)
{
    if (m_target != target) {
        m_target = target;
    }
}

Qt3DCore::QTransform *OrbitalCameraController::target() const
{
    return m_target;
}

void OrbitalCameraController::setRadius(float radius)
{
    if (!qFuzzyCompare(radius, m_radius)) {
        m_radius = radius;
        updateMatrix();
    }
}

float OrbitalCameraController::radius() const
{
    return m_radius;
}

void OrbitalCameraController::setAngle(float angle)
{
    if (!qFuzzyCompare(angle, m_angle)) {
        m_angle = angle;
        updateMatrix();
    }
}

float OrbitalCameraController::angle() const
{
    return m_angle;
}

void OrbitalCameraController::updateMatrix()
{
    m_matrix.setToIdentity();
    m_matrix.rotate(m_angle, QVector3D(0.0f, 1.0f, 0.0f));
    m_matrix.translate(m_radius, 0.0f, 0.0f);
    m_target->setMatrix(m_matrix);
}
