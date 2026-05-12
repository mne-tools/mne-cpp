//=============================================================================================================
/**
 * @file     polhemus_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Implementation of @ref UTILSLIB::PolhemusCoregistration.
 */

#include "polhemus_coregistration.h"

#include <cmath>

using namespace UTILSLIB;

//=============================================================================================================

PolhemusCoregistration::PolhemusCoregistration(QObject* parent)
    : QObject(parent)
    , m_pPoints(new AcquiredPoints(this))
{
    m_deviceToWorld.setToIdentity();
    m_headToWorld.setToIdentity();
    m_headToDevice.setToIdentity();
}

//=============================================================================================================

void PolhemusCoregistration::setTrackerStation(int station)
{
    m_trackerStation = station;
}

void PolhemusCoregistration::setPenStation(int station)
{
    m_penStation = station;
}

//=============================================================================================================

void PolhemusCoregistration::setTrackerToDeviceOffset(const QVector3D& translation,
                                                       const QQuaternion& rotation)
{
    m_offsetTranslation = translation;
    m_offsetRotation    = rotation;
}

//=============================================================================================================

void PolhemusCoregistration::setConnection(PolhemusConnection* conn)
{
    if (m_pConn) {
        disconnect(m_pConn, nullptr, this, nullptr);
    }
    m_pConn = conn;
    if (m_pConn) {
        connect(m_pConn, &PolhemusConnection::pointReceived,
                this,    &PolhemusCoregistration::onPointReceived);
        connect(m_pConn, &PolhemusConnection::penButtonPressed,
                this,    &PolhemusCoregistration::onPenButtonPressedFromConn);
    }
}

//=============================================================================================================

bool PolhemusCoregistration::captureCurrentPenPositionAsFiducial(FiducialId id)
{
    if (!m_havePenPos) {
        return false;
    }

    m_pPoints->removeFiducial(id);

    static const char* labels[] = { nullptr, "LPA", "NAS", "RPA" };
    const int ident = static_cast<int>(id);

    DigitizedPoint dp;
    dp.kind        = PointKind::Fiducial;
    dp.label       = QString::fromLatin1(labels[ident]);
    dp.identNumber = ident;
    dp.position    = m_penPosition;
    m_pPoints->append(dp);
    return true;
}

bool PolhemusCoregistration::captureCurrentPenPositionAsHeadShape()
{
    if (!m_havePenPos) {
        return false;
    }

    const int n = m_pPoints->countOf(PointKind::HeadShape) + 1;

    DigitizedPoint dp;
    dp.kind        = PointKind::HeadShape;
    dp.label       = QStringLiteral("HSP-%1").arg(n);
    dp.identNumber = n;
    dp.position    = m_penPosition;
    m_pPoints->append(dp);
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::resetRegistration()
{
    m_registrationValid = false;
    m_headToWorld.setToIdentity();
    m_headToDevice.setToIdentity();
    emit registrationChanged();
}

//=============================================================================================================

bool PolhemusCoregistration::computeRegistration()
{
    if (!m_pPoints->hasAllFiducials()) {
        return false;
    }

    const QMatrix4x4 headFrame = buildHeadFrame();

    const QVector3D ez(headFrame(0, 2), headFrame(1, 2), headFrame(2, 2));
    if (ez.length() < 1e-6f) {
        return false;
    }

    m_headToWorld      = headFrame;
    m_headToDevice     = m_deviceToWorld.inverted() * m_headToWorld;
    m_registrationValid = true;
    emit registrationChanged();
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::onPointReceived(int station,
                                              const QVector3D& position,
                                              const QQuaternion& orientation)
{
    if (station == m_trackerStation) {
        m_deviceToWorld = buildDevicePose(position, orientation);
        emit devicePoseChanged(m_deviceToWorld);
    } else if (station == m_penStation) {
        m_penPosition    = position;
        m_penOrientation = orientation;
        m_havePenPos     = true;
        emit penPoseChanged(m_penPosition, m_penOrientation);
    }
}

void PolhemusCoregistration::onPenButtonPressedFromConn(int station,
                                                         const QVector3D& position,
                                                         const QQuaternion& orientation)
{
    if (station == m_penStation) {
        m_penPosition    = position;
        m_penOrientation = orientation;
        m_havePenPos     = true;
        emit penButtonPressed(position, orientation);
    }
}

//=============================================================================================================

QMatrix4x4 PolhemusCoregistration::buildDevicePose(const QVector3D& trackerPos,
                                                    const QQuaternion& trackerOri) const
{
    QMatrix4x4 trackerToWorld;
    trackerToWorld.setToIdentity();
    trackerToWorld.translate(trackerPos);
    trackerToWorld.rotate(trackerOri);

    QMatrix4x4 offset;
    offset.setToIdentity();
    offset.translate(m_offsetTranslation);
    offset.rotate(m_offsetRotation);

    return trackerToWorld * offset;
}

QMatrix4x4 PolhemusCoregistration::buildHeadFrame() const
{
    const QVector3D nas = m_pPoints->fiducial(FiducialId::NAS);
    const QVector3D lpa = m_pPoints->fiducial(FiducialId::LPA);
    const QVector3D rpa = m_pPoints->fiducial(FiducialId::RPA);

    const QVector3D origin = (lpa + rpa) * 0.5f;
    const QVector3D ex     = (nas - origin).normalized();
    const QVector3D eyApprox = (lpa - origin).normalized();
    const QVector3D ez     = QVector3D::crossProduct(ex, eyApprox).normalized();
    const QVector3D ey     = QVector3D::crossProduct(ez, ex).normalized();

    QMatrix4x4 frame;
    frame.setToIdentity();
    frame(0, 0) = ex.x();  frame(0, 1) = ey.x();  frame(0, 2) = ez.x();  frame(0, 3) = origin.x();
    frame(1, 0) = ex.y();  frame(1, 1) = ey.y();  frame(1, 2) = ez.y();  frame(1, 3) = origin.y();
    frame(2, 0) = ex.z();  frame(2, 1) = ey.z();  frame(2, 2) = ez.z();  frame(2, 3) = origin.z();
    frame(3, 0) = 0.0f;    frame(3, 1) = 0.0f;    frame(3, 2) = 0.0f;    frame(3, 3) = 1.0f;
    return frame;
}
