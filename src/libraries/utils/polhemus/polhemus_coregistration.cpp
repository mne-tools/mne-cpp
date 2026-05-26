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

#include <Eigen/Dense>
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

    static const char* labels[] = { nullptr, "LPA", "NAS", "RPA" };
    const int ident = static_cast<int>(id);

    // Store pen fiducial BEFORE append (which emits pointsChanged)
    m_penFid[ident] = m_penPosition;
    m_hasPenFid[ident] = true;

    // Compute raw sensor position (reverse the tip offset)
    const QVector3D tipAdj = m_tipOffsetEnabled
        ? m_penOrientation.rotatedVector(m_penTipOffset) : QVector3D();
    const QVector3D rawSensor = m_penPosition - tipAdj;

    qInfo().nospace() << "captureFiducial: " << labels[ident]
        << "  tipPos=(" << m_penPosition.x()*1000.f << ", "
        << m_penPosition.y()*1000.f << ", " << m_penPosition.z()*1000.f << ") mm"
        << "  rawSensor=(" << rawSensor.x()*1000.f << ", "
        << rawSensor.y()*1000.f << ", " << rawSensor.z()*1000.f << ") mm"
        << "  ori=" << m_penOrientation
        << "  tipAdj=(" << tipAdj.x()*1000.f << ", "
        << tipAdj.y()*1000.f << ", " << tipAdj.z()*1000.f << ") mm";

    // Log distances to previously captured fiducials
    for (int j = 1; j <= 3; ++j) {
        if (j != ident && m_hasPenFid[j]) {
            float dist = (m_penFid[ident] - m_penFid[j]).length() * 1000.0f;
            qInfo().nospace() << "  distance " << labels[ident] << "-" << labels[j]
                << " = " << dist << " mm"
                << (dist > 200.0f ? "  *** WARNING: > 200mm, likely wrong capture!" : "");
        }
    }

    m_pPoints->removeFiducial(id);

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
    m_worldToModel.setToIdentity();
    m_hasPenVertex = false;
    for (int i = 0; i < 4; ++i) m_hasPenFid[i] = false;
    emit registrationChanged();
}

//=============================================================================================================

void PolhemusCoregistration::setModelFiducial(FiducialId id, const QVector3D& posInModel)
{
    const int i = static_cast<int>(id);
    m_modelFid[i] = posInModel;
    m_hasModelFid[i] = true;
}

bool PolhemusCoregistration::hasModelFiducial(FiducialId id) const
{
    return m_hasModelFid[static_cast<int>(id)];
}

bool PolhemusCoregistration::hasAllModelFiducials() const
{
    return m_hasModelFid[1] && m_hasModelFid[2] && m_hasModelFid[3];
}

QVector3D PolhemusCoregistration::modelFiducial(FiducialId id) const
{
    return m_modelFid[static_cast<int>(id)];
}

bool PolhemusCoregistration::hasAllPenFiducials() const
{
    return m_hasPenFid[1] && m_hasPenFid[2] && m_hasPenFid[3];
}

//=============================================================================================================

bool PolhemusCoregistration::captureCurrentPenPositionAsVertex()
{
    if (!m_havePenPos) return false;
    m_penVertex = m_penPosition;
    m_hasPenVertex = true;
    qInfo() << "Captured pen vertex (CZ) at" << m_penVertex * 1000.0f << "mm";
    return true;
}

//=============================================================================================================

bool PolhemusCoregistration::computeRegistration()
{
    if (!hasAllPenFiducials()) {
        return false;
    }

    // Pen fiducial positions in Polhemus world frame (metres)
    const QVector3D pNas = m_penFid[static_cast<int>(FiducialId::NAS)];
    const QVector3D pLpa = m_penFid[static_cast<int>(FiducialId::LPA)];
    const QVector3D pRpa = m_penFid[static_cast<int>(FiducialId::RPA)];

    // Spread check: reject degenerate input
    const float dNL = (pNas - pLpa).length() * 1000.0f;
    const float dNR = (pNas - pRpa).length() * 1000.0f;
    const float dLR = (pLpa - pRpa).length() * 1000.0f;
    const float minSpread = std::min({dNL, dNR, dLR});
    qInfo() << "computeRegistration: pen spread NAS-LPA" << dNL
            << "NAS-RPA" << dNR << "LPA-RPA" << dLR << "mm";
    if (minSpread < 20.0f) {
        qWarning() << "computeRegistration: DEGENERATE — pen fiducials too close"
                   << "(min spread" << minSpread << "mm, need >20 mm)";
        m_registrationValid = false;
        emit registrationChanged();
        return false;
    }

    // --- Paired path: analytical head-frame alignment ---
    if (hasAllModelFiducials()) {
        const QVector3D mNas = m_modelFid[static_cast<int>(FiducialId::NAS)];
        const QVector3D mLpa = m_modelFid[static_cast<int>(FiducialId::LPA)];
        const QVector3D mRpa = m_modelFid[static_cast<int>(FiducialId::RPA)];

        // Compare pen vs model fiducial distances — should be similar for the same head
        const float mNL = (mNas - mLpa).length() * 1000.0f;
        const float mNR = (mNas - mRpa).length() * 1000.0f;
        const float mLR = (mLpa - mRpa).length() * 1000.0f;
        qInfo().nospace() << "computeRegistration: distance comparison (pen / model):"
            << "\n  NAS-LPA: " << dNL << " / " << mNL << " mm  (ratio " << dNL/mNL << ")"
            << "\n  NAS-RPA: " << dNR << " / " << mNR << " mm  (ratio " << dNR/mNR << ")"
            << "\n  LPA-RPA: " << dLR << " / " << mLR << " mm  (ratio " << dLR/mLR << ")";

        const float maxRatio = std::max({dNL/mNL, dNR/mNR, dLR/mLR});
        const float minRatio = std::min({dNL/mNL, dNR/mNR, dLR/mLR});
        if (maxRatio / minRatio > 2.0f) {
            qWarning() << "computeRegistration: *** SHAPE MISMATCH — pen fiducial triangle"
                       << "has very different proportions from model fiducials."
                       << "One or more pen fiducials may be captured at the wrong location!";
        }

        // Build a head coordinate frame from 3 fiducials + optional vertex.
        // Uses Gram-Schmidt to orthogonalize ey (toward LPA) against ex
        // first, then derives ez via cross product.  This guarantees ey
        // always points toward LPA regardless of coordinate system
        // orientation, preventing left-right inversion.
        auto buildFrame = [](const QVector3D& nas, const QVector3D& lpa, const QVector3D& rpa,
                             const QVector3D* vertex) -> QMatrix4x4
        {
            const QVector3D origin = (lpa + rpa) * 0.5f;
            const QVector3D ex     = (nas - origin).normalized();
            const QVector3D eyApp  = (lpa - origin).normalized();

            // Gram-Schmidt: remove component of eyApp along ex
            const QVector3D ey = (eyApp - QVector3D::dotProduct(eyApp, ex) * ex).normalized();
            // ez from right-hand rule: always consistent with ey toward LPA
            QVector3D ez = QVector3D::crossProduct(ex, ey).normalized();

            // Optional validation: warn if ez disagrees with vertex direction
            if (vertex) {
                const QVector3D toVertex = (*vertex - origin).normalized();
                if (QVector3D::dotProduct(ez, toVertex) < 0.0f) {
                    qWarning() << "  head-frame: ez points away from vertex"
                               << "— check fiducial digitization";
                }
            }

            QMatrix4x4 f;
            f.setToIdentity();
            f(0,0) = ex.x();  f(0,1) = ey.x();  f(0,2) = ez.x();  f(0,3) = origin.x();
            f(1,0) = ex.y();  f(1,1) = ey.y();  f(1,2) = ez.y();  f(1,3) = origin.y();
            f(2,0) = ex.z();  f(2,1) = ey.z();  f(2,2) = ez.z();  f(2,3) = origin.z();
            return f;
        };

        const QMatrix4x4 penFrame   = buildFrame(pNas, pLpa, pRpa,
                                                    m_hasPenVertex ? &m_penVertex : nullptr);
        const QMatrix4x4 modelFrame = buildFrame(mNas, mLpa, mRpa,
                                                    m_hasModelVertex ? &m_modelVertex : nullptr);

        // Debug: dump both frames and fiducial inputs
        qInfo() << "computeRegistration: penFiducials (mm)"
                << "NAS" << pNas * 1000.0f << "LPA" << pLpa * 1000.0f << "RPA" << pRpa * 1000.0f;
        qInfo() << "computeRegistration: modelFiducials (mm)"
                << "NAS" << mNas * 1000.0f << "LPA" << mLpa * 1000.0f << "RPA" << mRpa * 1000.0f;
        qInfo() << "computeRegistration: penFrame\n" << penFrame;
        qInfo() << "computeRegistration: modelFrame\n" << modelFrame;

        m_worldToModel = modelFrame * penFrame.inverted();

        qInfo() << "computeRegistration: worldToModel\n" << m_worldToModel;

        // Residuals — log both mapped and target positions for cross-checking
        auto residual = [&](const char* label, const QVector3D& pw, const QVector3D& pm) {
            QVector3D mapped = m_worldToModel.map(pw);
            float err = (mapped - pm).length() * 1000.0f;
            qInfo().nospace() << "  " << label
                << ": pen(world)=" << pw * 1000.0f
                << " → mapped=" << mapped * 1000.0f
                << "  model=" << pm * 1000.0f
                << "  residual=" << err << " mm";
            return err;
        };
        residual("NAS", pNas, mNas);
        residual("LPA", pLpa, mLpa);
        residual("RPA", pRpa, mRpa);

        m_headToWorld.setToIdentity();
        m_headToDevice = m_deviceToWorld.inverted() * m_headToWorld;
        m_registrationValid = true;
        qInfo() << "computeRegistration: analytical paired succeeded";
        emit registrationChanged();
        return true;
    }

    // --- Fallback: head-frame-only registration ---
    const QMatrix4x4 headFrame = buildHeadFrame();
    const QVector3D ez(headFrame(0, 2), headFrame(1, 2), headFrame(2, 2));
    if (ez.length() < 1e-6f) {
        return false;
    }

    m_headToWorld       = headFrame;
    m_headToDevice      = m_deviceToWorld.inverted() * m_headToWorld;
    m_worldToModel.setToIdentity();
    m_registrationValid = true;
    qInfo() << "computeRegistration: head-frame fallback succeeded";
    emit registrationChanged();
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::onPointReceived(int station,
                                              const QVector3D& position,
                                              const QQuaternion& orientation)
{
    // Periodic debug logging
    static QHash<int, int> sampleCount;
    int& n = sampleCount[station];
    if (n == 0) {
        qInfo() << "onPointReceived: FIRST sample station" << station
                << "pos(mm)" << position * 1000.0f
                << "ori" << orientation
                << "hasOri" << !orientation.isIdentity();
    }
    ++n;

    // Log every ~10 s per station (~600 samples at 60 Hz)
    if (n % 600 == 0) {
        const float sinEl = 2.0f * (orientation.scalar() * orientation.y()
                                  - orientation.x() * orientation.z());
        const float elDeg = std::asin(std::clamp(sinEl, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
        if (station == m_penStation && m_tipOffsetEnabled) {
            const QVector3D tip = position + orientation.rotatedVector(m_penTipOffset);
            if (m_registrationValid) {
                const QVector3D mapped = m_worldToModel.map(tip);
                qInfo().nospace() << "pen #" << n
                    << "  raw(" << position.x()*1000.f << ", " << position.y()*1000.f << ", " << position.z()*1000.f << ") mm"
                    << "  tip(" << tip.x()*1000.f << ", " << tip.y()*1000.f << ", " << tip.z()*1000.f << ") mm"
                    << "  →model(" << mapped.x()*1000.f << ", " << mapped.y()*1000.f << ", " << mapped.z()*1000.f << ") mm"
                    << "  el=" << elDeg << "\u00b0";
            } else {
                qInfo().nospace() << "pen #" << n
                    << "  raw(" << position.x()*1000.f << ", " << position.y()*1000.f << ", " << position.z()*1000.f << ") mm"
                    << "  tip(" << tip.x()*1000.f << ", " << tip.y()*1000.f << ", " << tip.z()*1000.f << ") mm"
                    << "  el=" << elDeg << "\u00b0";
            }
        } else if (station == m_trackerStation && m_registrationValid) {
            const QVector3D trkWorld(m_deviceToWorld(0,3), m_deviceToWorld(1,3), m_deviceToWorld(2,3));
            const QVector3D mapped = m_worldToModel.map(trkWorld);
            qInfo().nospace() << "st" << station << " #" << n
                << "  pos(" << position.x()*1000.f << ", " << position.y()*1000.f << ", " << position.z()*1000.f << ") mm"
                << "  →model(" << mapped.x()*1000.f << ", " << mapped.y()*1000.f << ", " << mapped.z()*1000.f << ") mm"
                << "  el=" << elDeg << "\u00b0";
        } else {
            qInfo().nospace() << "st" << station << " #" << n
                << "  pos(" << position.x()*1000.f << ", " << position.y()*1000.f << ", " << position.z()*1000.f << ") mm"
                << "  el=" << elDeg << "\u00b0";
        }
    }
    if (station == m_trackerStation) {
        m_deviceToWorld = buildDevicePose(position, orientation);
        emit devicePoseChanged(m_deviceToWorld);
    } else if (station == m_penStation) {
        // Gimbal-lock guard: for ZYX Euler, sin(el) = 2*(w*y - x*z).
        // When |el| > 80° the Euler→quaternion conversion is unreliable,
        // so freeze the pen pose at its last good value.
        const float sinEl = 2.0f * (orientation.scalar() * orientation.y()
                                  - orientation.x() * orientation.z());
        constexpr float kGimbalSinEl = 0.9848f; // sin(80°)
        const bool gimbalLock = (std::abs(sinEl) > kGimbalSinEl);

        if (!gimbalLock) {
            const QVector3D tipAdj = m_tipOffsetEnabled
                ? orientation.rotatedVector(m_penTipOffset) : QVector3D();
            m_penPosition    = position + tipAdj;
            m_penOrientation = orientation;
            m_havePenPos     = true;
        }

        // Collect raw (un-offset) samples during pivot calibration.
        // Only keep samples with sufficient angular change from the last
        // accepted sample to avoid redundant near-identical rows in the SVD.
        // Also reject position jumps and gimbal-lock orientations.
        if (m_pivotState == PivotState::Collecting) {
            constexpr float kMinAngleDeg = 3.0f;
            constexpr float kMaxPosJumpM = 0.05f; // 5 cm
            constexpr float kGimbalSinEl = 0.9848f; // sin(80°) — reject |el|>80°

            // Gimbal-lock check: for ZYX Euler, sin(el) = 2*(w*y - x*z)
            const float sinEl = 2.0f * (orientation.scalar() * orientation.y()
                                      - orientation.x() * orientation.z());
            if (std::abs(sinEl) > kGimbalSinEl) {
                // Near gimbal lock — skip this sample silently
            } else {
                bool accept = m_pivotOrientations.empty();
                if (!accept) {
                    const QQuaternion& prev = m_pivotOrientations.back();
                    float dot = std::abs(QQuaternion::dotProduct(prev, orientation));
                    float angleDeg = 2.0f * std::acos(std::min(dot, 1.0f)) * (180.0f / 3.14159265f);
                    float posDelta = (position - m_pivotPositions.back()).length();
                    accept = (angleDeg >= kMinAngleDeg) && (posDelta < kMaxPosJumpM);
                }
                if (accept) {
                    m_pivotPositions.push_back(position);
                    m_pivotOrientations.push_back(orientation);
                }
            }
        }

        emit penPoseChanged(m_penPosition, m_penOrientation);
    }
}

void PolhemusCoregistration::onPenButtonPressedFromConn(int station,
                                                         const QVector3D& position,
                                                         const QQuaternion& orientation)
{
    if (station == m_penStation) {
        const QVector3D tipAdj = m_tipOffsetEnabled
            ? orientation.rotatedVector(m_penTipOffset) : QVector3D();
        m_penPosition    = position + tipAdj;
        m_penOrientation = orientation;
        m_havePenPos     = true;

        // Pivot calibration state machine
        if (m_pivotState == PivotState::WaitingForStart) {
            m_pivotState = PivotState::Collecting;
            m_pivotPositions.clear();
            m_pivotOrientations.clear();
            qInfo() << "Pivot calibration: collecting — pivot the pen around its tip";
            emit pivotStateChanged(m_pivotState);
            return;
        }
        if (m_pivotState == PivotState::Collecting) {
            solvePivotCalibration();
            return;
        }

        emit penButtonPressed(m_penPosition, orientation);
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
    // Gram-Schmidt: orthogonalize ey against ex, preserving LPA direction
    const QVector3D ey     = (eyApprox - QVector3D::dotProduct(eyApprox, ex) * ex).normalized();
    const QVector3D ez     = QVector3D::crossProduct(ex, ey).normalized();

    QMatrix4x4 frame;
    frame.setToIdentity();
    frame(0, 0) = ex.x();  frame(0, 1) = ey.x();  frame(0, 2) = ez.x();  frame(0, 3) = origin.x();
    frame(1, 0) = ex.y();  frame(1, 1) = ey.y();  frame(1, 2) = ez.y();  frame(1, 3) = origin.y();
    frame(2, 0) = ex.z();  frame(2, 1) = ey.z();  frame(2, 2) = ez.z();  frame(2, 3) = origin.z();
    frame(3, 0) = 0.0f;    frame(3, 1) = 0.0f;    frame(3, 2) = 0.0f;    frame(3, 3) = 1.0f;
    return frame;
}

//=============================================================================================================
// Pivot calibration
//=============================================================================================================

void PolhemusCoregistration::startPivotCalibration()
{
    m_pivotPositions.clear();
    m_pivotOrientations.clear();
    m_pivotResidualMm = 0.0f;
    m_pivotState = PivotState::WaitingForStart;
    qInfo() << "Pivot calibration: press stylus button to begin collecting";
    emit pivotStateChanged(m_pivotState);
}

void PolhemusCoregistration::cancelPivotCalibration()
{
    m_pivotPositions.clear();
    m_pivotOrientations.clear();
    m_pivotState = PivotState::Idle;
    emit pivotStateChanged(m_pivotState);
}

bool PolhemusCoregistration::solvePivotCalibration()
{
    const int N = static_cast<int>(m_pivotPositions.size());
    if (N < 10) {
        qWarning() << "Pivot calibration: only" << N << "samples, need at least 10";
        m_pivotState = PivotState::Idle;
        emit pivotStateChanged(m_pivotState);
        return false;
    }

    // Build the linear system  A * x = b  (double precision)
    // where x = [offset(3); tipPos(3)]
    // For each sample i:  R_i * offset - I * tipPos = -p_i
    Eigen::MatrixXd A(3 * N, 6);
    Eigen::VectorXd b(3 * N);

    for (int i = 0; i < N; ++i) {
        const QVector3D& p = m_pivotPositions[static_cast<size_t>(i)];
        const QQuaternion& q = m_pivotOrientations[static_cast<size_t>(i)];

        // Extract 3x3 rotation matrix directly from quaternion
        const QMatrix3x3 rm = q.toRotationMatrix();

        const int row = 3 * i;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                A(row + r, c)     = static_cast<double>(rm(r, c));   // R_i
                A(row + r, 3 + c) = (r == c) ? -1.0 : 0.0;          // -I
            }
        }
        b(row + 0) = -static_cast<double>(p.x());
        b(row + 1) = -static_cast<double>(p.y());
        b(row + 2) = -static_cast<double>(p.z());
    }

    // Solve via SVD least squares
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);

    // Check condition number for rank deficiency (insufficient angular diversity)
    const auto& sv = svd.singularValues();
    double cond = sv(0) / sv(sv.size() - 1);
    if (cond > 1e6) {
        qWarning() << "Pivot calibration: ill-conditioned (cond =" << cond
                   << "). Pivot the pen through a wider range of angles.";
        m_pivotState = PivotState::Idle;
        emit pivotStateChanged(m_pivotState);
        return false;
    }

    Eigen::VectorXd x = svd.solve(b);

    QVector3D offset(static_cast<float>(x(0)),
                     static_cast<float>(x(1)),
                     static_cast<float>(x(2)));
    QVector3D tipPos(static_cast<float>(x(3)),
                     static_cast<float>(x(4)),
                     static_cast<float>(x(5)));

    // Compute RMS residual
    double sumSq = 0.0;
    for (int i = 0; i < N; ++i) {
        const QVector3D& p = m_pivotPositions[static_cast<size_t>(i)];
        const QQuaternion& q = m_pivotOrientations[static_cast<size_t>(i)];
        QVector3D computed = p + q.rotatedVector(offset);
        float err = (computed - tipPos).length();
        sumSq += static_cast<double>(err) * static_cast<double>(err);
    }
    float rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(N))) * 1000.0f;
    m_pivotResidualMm = rms;

    qInfo() << "Pivot calibration:" << N << "samples, cond =" << cond
            << ", offset =" << offset.x() * 1000.0f << offset.y() * 1000.0f
            << offset.z() * 1000.0f << "mm, RMS residual =" << rms << "mm";

    m_penTipOffset = offset;
    m_pivotState = PivotState::Done;
    emit pivotStateChanged(m_pivotState);
    emit pivotCalibrationDone(offset, rms);
    return true;
}
