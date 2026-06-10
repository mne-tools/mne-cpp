//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref UTILSLIB::PolhemusCoregistration — fiducial-based head→device rigid alignment.
 *
 * The three cardinal fiducials (LPA, NAS, RPA) define an
 * orthonormal head frame whose construction matches the
 * MEG/EEG convention used throughout mne-cpp. The head→device
 * transform is obtained by composing that frame with the pre-
 * calibrated tracker-to-device offset and with the live tracker
 * pose streamed from the Polhemus connection, all wrapped in
 * @c QMatrix4x4 so callers can hand the result straight to the
 * OpenGL viewer in DISP3DLIB or to the navigation export.
 */

#include "polhemus_coregistration.h"
#include "acquired_points.h"

#include <Eigen/Dense>
#include <QSettings>
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

    qInfo().nospace() << "Fiducial captured: " << labels[ident]
        << "  (" << m_penPosition.x()*1000.f << ", "
        << m_penPosition.y()*1000.f << ", "
        << m_penPosition.z()*1000.f << ") mm";

    // Log distances to previously captured fiducials
    for (int j = 1; j <= 3; ++j) {
        if (j != ident && m_hasPenFid[j]) {
            float dist = (m_penFid[ident] - m_penFid[j]).length() * 1000.0f;
            qInfo().nospace() << "  " << labels[ident] << " ↔ " << labels[j]
                << ": " << dist << " mm"
                << (dist > 200.0f ? "  *** WARNING: > 200 mm!" : "");
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
    // Notify observers so auto-registration can trigger
    if (m_pPoints)
        emit m_pPoints->pointsChanged();
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
    if (minSpread < 20.0f) {
        qWarning() << "Registration failed: pen fiducials too close"
                   << "(min spread" << minSpread << "mm, need > 20 mm)";
        m_registrationValid = false;
        emit registrationChanged();
        return false;
    }

    // --- Paired path: SVD-based rigid registration (Kabsch / Procrustes) ---
    //
    // Finds the optimal rotation R and translation t that map pen
    // fiducials to model fiducials:  model_pos ≈ R * pen_pos + t
    //
    // The Kabsch algorithm guarantees det(R) = +1 (proper rotation,
    // no reflection) regardless of how the two coordinate systems are
    // oriented.  This avoids the left-right / up-down inversion bugs
    // that plagued the old buildFrame approach, where an asymmetric
    // vertex correction could flip ey or ez in one frame but not the
    // other.
    if (hasAllModelFiducials()) {
        const QVector3D mNas = m_modelFid[static_cast<int>(FiducialId::NAS)];
        const QVector3D mLpa = m_modelFid[static_cast<int>(FiducialId::LPA)];
        const QVector3D mRpa = m_modelFid[static_cast<int>(FiducialId::RPA)];

        // Debug: raw fiducial positions
        qInfo().nospace()
            << "Registration — pen fiducial positions (Polhemus world, mm):"
            << "\n    NAS: (" << pNas.x()*1000.f << ", " << pNas.y()*1000.f << ", " << pNas.z()*1000.f << ")"
            << "\n    LPA: (" << pLpa.x()*1000.f << ", " << pLpa.y()*1000.f << ", " << pLpa.z()*1000.f << ")"
            << "\n    RPA: (" << pRpa.x()*1000.f << ", " << pRpa.y()*1000.f << ", " << pRpa.z()*1000.f << ")";
        qInfo().nospace()
            << "Registration — model fiducial positions (MRI/surface-RAS, mm):"
            << "\n    NAS: (" << mNas.x()*1000.f << ", " << mNas.y()*1000.f << ", " << mNas.z()*1000.f << ")"
            << "\n    LPA: (" << mLpa.x()*1000.f << ", " << mLpa.y()*1000.f << ", " << mLpa.z()*1000.f << ")"
            << "\n    RPA: (" << mRpa.x()*1000.f << ", " << mRpa.y()*1000.f << ", " << mRpa.z()*1000.f << ")";
        qInfo().nospace()
            << "Registration — axis mirror: mirrorX=" << m_mirrorX << " mirrorY=" << m_mirrorY;

        // Compare pen vs model fiducial distances — should be similar for the same head
        const float mNL = (mNas - mLpa).length() * 1000.0f;
        const float mNR = (mNas - mRpa).length() * 1000.0f;
        const float mLR = (mLpa - mRpa).length() * 1000.0f;
        qInfo().nospace()
            << "Registration — fiducial distances (pen / model):"
            << "\n    NAS ↔ LPA: " << dNL << " / " << mNL << " mm  (ratio " << dNL/mNL << ")"
            << "\n    NAS ↔ RPA: " << dNR << " / " << mNR << " mm  (ratio " << dNR/mNR << ")"
            << "\n    LPA ↔ RPA: " << dLR << " / " << mLR << " mm  (ratio " << dLR/mLR << ")";

        const float maxRatio = std::max({dNL/mNL, dNR/mNR, dLR/mLR});
        const float minRatio = std::min({dNL/mNL, dNR/mNR, dLR/mLR});
        if (maxRatio / minRatio > 2.0f) {
            qWarning() << "Registration: shape mismatch — pen fiducial triangle"
                       << "has very different proportions from model."
                       << "Check fiducial placement!";
        }

        // --- Kabsch algorithm (SVD least-squares rigid alignment) ---

        // 1. Centroids
        const Eigen::Vector3d penC(
            (pNas.x() + pLpa.x() + pRpa.x()) / 3.0,
            (pNas.y() + pLpa.y() + pRpa.y()) / 3.0,
            (pNas.z() + pLpa.z() + pRpa.z()) / 3.0);
        const Eigen::Vector3d modC(
            (mNas.x() + mLpa.x() + mRpa.x()) / 3.0,
            (mNas.y() + mLpa.y() + mRpa.y()) / 3.0,
            (mNas.z() + mLpa.z() + mRpa.z()) / 3.0);

        // 2. Centered point matrices (3×3, each column = one centered point)
        Eigen::Matrix3d P, Q;
        P.col(0) = Eigen::Vector3d(pNas.x(), pNas.y(), pNas.z()) - penC;
        P.col(1) = Eigen::Vector3d(pLpa.x(), pLpa.y(), pLpa.z()) - penC;
        P.col(2) = Eigen::Vector3d(pRpa.x(), pRpa.y(), pRpa.z()) - penC;

        Q.col(0) = Eigen::Vector3d(mNas.x(), mNas.y(), mNas.z()) - modC;
        Q.col(1) = Eigen::Vector3d(mLpa.x(), mLpa.y(), mLpa.z()) - modC;
        Q.col(2) = Eigen::Vector3d(mRpa.x(), mRpa.y(), mRpa.z()) - modC;

        // 3. Cross-covariance matrix H = P * Qᵀ
        const Eigen::Matrix3d H = P * Q.transpose();

        // 4. SVD of H
        Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
        const Eigen::Matrix3d U = svd.matrixU();
        const Eigen::Matrix3d V = svd.matrixV();

        // 5. Optimal rotation — ensure proper rotation (det = +1)
        const double d = (V * U.transpose()).determinant();
        Eigen::Matrix3d D = Eigen::Matrix3d::Identity();
        D(2, 2) = (d > 0.0) ? 1.0 : -1.0;
        Eigen::Matrix3d R = V * D * U.transpose();

        // 6. Translation
        Eigen::Vector3d t = modC - R * penC;

        const Eigen::Vector3d sv = svd.singularValues();
        qInfo().nospace()
            << "Registration — Kabsch SVD:"
            << "\n    det(V*Uᵀ) = " << d << (d < 0 ? " (det correction applied)" : " (no det correction)")
            << "\n    det(R) = " << R.determinant()
            << "\n    singular values: (" << sv(0) << ", " << sv(1) << ", " << sv(2) << ")";

        // 6b. Vertex disambiguation for coplanar degeneracy.
        //
        // With only 3 coplanar fiducials the SVD's 3rd singular value
        // is ~0, leaving the out-of-plane rotation direction ambiguous.
        // The det(V*Uᵀ) sign heuristic picks one direction but may
        // choose wrong, inverting superior ↔ inferior.
        //
        // Fix: if a pen vertex (CZ) and model vertex are available,
        // test both candidate rotations (D₃₃ = +1 and D₃₃ = -1) and
        // keep whichever maps pen CZ closer to model CZ.
        if (m_hasPenVertex && m_hasModelVertex) {
            const Eigen::Vector3d penCZ(m_penVertex.x(), m_penVertex.y(), m_penVertex.z());
            const Eigen::Vector3d modCZ(m_modelVertex.x(), m_modelVertex.y(), m_modelVertex.z());

            const Eigen::Vector3d mappedCZ = R * penCZ + t;
            const double errCurrent = (mappedCZ - modCZ).norm();

            // Try the alternative sign
            Eigen::Matrix3d D_alt = D;
            D_alt(2, 2) = -D(2, 2);
            const Eigen::Matrix3d R_alt = V * D_alt * U.transpose();
            const Eigen::Vector3d t_alt = modC - R_alt * penC;
            const Eigen::Vector3d mappedCZ_alt = R_alt * penCZ + t_alt;
            const double errAlt = (mappedCZ_alt - modCZ).norm();

            qInfo().nospace()
                << "Registration — vertex disambiguation:"
                << "\n    CZ (pen):   (" << m_penVertex.x()*1000.f << ", " << m_penVertex.y()*1000.f << ", " << m_penVertex.z()*1000.f << ") mm"
                << "\n    CZ (model): (" << m_modelVertex.x()*1000.f << ", " << m_modelVertex.y()*1000.f << ", " << m_modelVertex.z()*1000.f << ") mm"
                << "\n    D₃₃=" << D(2,2) << " → mapped CZ err: " << errCurrent*1000.0 << " mm"
                << "\n    D₃₃=" << D_alt(2,2) << " → mapped CZ err: " << errAlt*1000.0 << " mm";

            if (errAlt < errCurrent) {
                R = R_alt;
                t = t_alt;
                D = D_alt;
                qInfo().nospace()
                    << "Registration — vertex correction APPLIED: flipped out-of-plane direction"
                    << " (det(R) now = " << R.determinant() << ")";
            } else {
                qInfo() << "Registration — vertex check passed, no flip needed.";
            }
        } else {
            qInfo() << "Registration — no vertex available for out-of-plane disambiguation."
                    << "Capture CZ (pen vertex) and load BEM to enable this check.";
        }

        // 7. Build QMatrix4x4 worldToModel = [R | t]
        m_worldToModel.setToIdentity();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                m_worldToModel(r, c) = static_cast<float>(R(r, c));
            }
            m_worldToModel(r, 3) = static_cast<float>(t(r));
        }

        // Residuals
        auto residual = [&](const char* label, const QVector3D& pw, const QVector3D& pm) {
            QVector3D mapped = m_worldToModel.map(pw);
            float err = (mapped - pm).length() * 1000.0f;
            qInfo().nospace() << "  " << label
                << ":  pen(" << pw.x()*1000.f << ", " << pw.y()*1000.f << ", " << pw.z()*1000.f << ")"
                << " → mapped(" << mapped.x()*1000.f << ", " << mapped.y()*1000.f << ", " << mapped.z()*1000.f << ")"
                << "  model(" << pm.x()*1000.f << ", " << pm.y()*1000.f << ", " << pm.z()*1000.f << ")"
                << "  err=" << err << " mm";
            return err;
        };
        const float rNas = residual("NAS", pNas, mNas);
        const float rLpa = residual("LPA", pLpa, mLpa);
        const float rRpa = residual("RPA", pRpa, mRpa);

        m_headToWorld.setToIdentity();
        m_headToDevice = m_deviceToWorld.inverted() * m_headToWorld;
        m_registrationValid = true;
        qInfo() << "Registration succeeded (analytical paired).";
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
    qInfo() << "Registration succeeded (head-frame fallback).";
    emit registrationChanged();
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::onPointReceived(int station,
                                              const QVector3D& position,
                                              const QQuaternion& orientation)
{
    // Apply axis mirroring to compensate for transmitter placement
    const QVector3D pos(m_mirrorX ? -position.x() : position.x(),
                        m_mirrorY ? -position.y() : position.y(),
                        position.z());

    if (station == m_trackerStation) {
        m_deviceToWorld = buildDevicePose(pos, orientation);
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
            m_penPosition    = pos + tipAdj;
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
            constexpr float kGimbalSinEl2 = 0.9848f; // sin(80°) — reject |el|>80°

            // Gimbal-lock check: for ZYX Euler, sin(el) = 2*(w*y - x*z)
            const float sinEl2 = 2.0f * (orientation.scalar() * orientation.y()
                                       - orientation.x() * orientation.z());
            if (std::abs(sinEl2) > kGimbalSinEl2) {
                // Near gimbal lock — skip this sample silently
            } else {
                bool accept = m_pivotOrientations.empty();
                if (!accept) {
                    const QQuaternion& prev = m_pivotOrientations.back();
                    float dot = std::abs(QQuaternion::dotProduct(prev, orientation));
                    float angleDeg = 2.0f * std::acos(std::min(dot, 1.0f)) * (180.0f / 3.14159265f);
                    float posDelta = (pos - m_pivotPositions.back()).length();
                    accept = (angleDeg >= kMinAngleDeg) && (posDelta < kMaxPosJumpM);
                }
                if (accept) {
                    m_pivotPositions.push_back(pos);
                    m_pivotOrientations.push_back(orientation);

                    // Compute angular span for live feedback
                    float spanDeg = 0.0f;
                    if (m_pivotOrientations.size() > 1) {
                        float minDot = 1.0f;
                        const auto& first = m_pivotOrientations.front();
                        for (size_t k = 1; k < m_pivotOrientations.size(); ++k) {
                            float d = std::abs(QQuaternion::dotProduct(first, m_pivotOrientations[k]));
                            if (d < minDot) minDot = d;
                        }
                        spanDeg = 2.0f * std::acos(std::min(minDot, 1.0f)) * (180.0f / 3.14159265f);
                    }
                    emit pivotSampleCollected(static_cast<int>(m_pivotPositions.size()), spanDeg);
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
        // Apply axis mirroring to compensate for transmitter placement
        const QVector3D pos(m_mirrorX ? -position.x() : position.x(),
                            m_mirrorY ? -position.y() : position.y(),
                            position.z());

        const QVector3D tipAdj = m_tipOffsetEnabled
            ? orientation.rotatedVector(m_penTipOffset) : QVector3D();
        m_penPosition    = pos + tipAdj;
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

//=============================================================================================================
// Optical path calibration
//=============================================================================================================

bool PolhemusCoregistration::captureOpticalCalibSample()
{
    if (!m_havePenPos) {
        qWarning() << "Optical calibration: no pen position available";
        return false;
    }

    const QMatrix4x4& dev = m_deviceToWorld;
    if (dev.isIdentity()) {
        qWarning() << "Optical calibration: no tracker data available";
        return false;
    }

    // Extract tracker position and orientation from deviceToWorld.
    // deviceToWorld = trackerToWorld * offset, but we want the raw tracker
    // pose (before offset). Reconstruct from the current raw tracker data
    // by removing the offset: trackerToWorld = deviceToWorld * offset^-1
    QMatrix4x4 offsetInv;
    offsetInv.setToIdentity();
    offsetInv.translate(m_offsetTranslation);
    offsetInv.rotate(m_offsetRotation);
    offsetInv = offsetInv.inverted();

    const QMatrix4x4 trackerToWorld = dev * offsetInv;
    const QVector3D trackerPos(trackerToWorld(0, 3), trackerToWorld(1, 3), trackerToWorld(2, 3));
    const QQuaternion trackerOri = QQuaternion::fromRotationMatrix(trackerToWorld.toGenericMatrix<3, 3>());

    OpticalCalibSample sample;
    sample.trackerPos = trackerPos;
    sample.trackerOri = trackerOri;
    sample.focusPoint = m_penPosition;
    m_opticalCalibSamples.push_back(sample);

    const int n = static_cast<int>(m_opticalCalibSamples.size());
    const QVector3D localFocus = trackerOri.inverted().rotatedVector(m_penPosition - trackerPos);
    const float distToTracker = localFocus.length() * 1000.0f;

    qInfo().nospace()
        << "Optical calibration sample " << n << ":"
        << "\n    tracker pos: (" << trackerPos.x()*1000.f << ", " << trackerPos.y()*1000.f << ", " << trackerPos.z()*1000.f << ") mm"
        << "\n    focus point: (" << m_penPosition.x()*1000.f << ", " << m_penPosition.y()*1000.f << ", " << m_penPosition.z()*1000.f << ") mm"
        << "\n    focus (local): (" << localFocus.x()*1000.f << ", " << localFocus.y()*1000.f << ", " << localFocus.z()*1000.f << ") mm"
        << "\n    distance tracker\u2194focus: " << distToTracker << " mm";

    // Log convergence angle between tracker→focus directions for successive samples.
    // As the OPMI moves farther away, this angle shrinks (lines become parallel);
    // at close range the angle is large because the ~20 cm tracker offset dominates.
    if (n >= 2) {
        const auto& prev = m_opticalCalibSamples[static_cast<size_t>(n - 2)];
        const QVector3D prevLocal = prev.trackerOri.inverted().rotatedVector(prev.focusPoint - prev.trackerPos);
        const QVector3D curDir  = localFocus.normalized();
        const QVector3D prevDir = prevLocal.normalized();
        const float dot = std::clamp(QVector3D::dotProduct(curDir, prevDir), -1.0f, 1.0f);
        const float angleDeg = std::acos(dot) * (180.0f / 3.14159265f);
        const float prevDist = prevLocal.length() * 1000.0f;
        qInfo().nospace()
            << "    convergence angle (sample " << n-1 << "\u2194" << n << "): " << angleDeg << "\u00b0"
            << "  (distances: " << prevDist << " / " << distToTracker << " mm)";
    }

    return true;
}

void PolhemusCoregistration::clearOpticalCalibSamples()
{
    m_opticalCalibSamples.clear();
    m_opticalCalibValid = false;
    m_opticalCalibResidualMm = 0.0f;
    m_opticalCalibDepthSpreadMm = 0.0f;
    emit opticalCalibrationChanged();
}

//=============================================================================================================

bool PolhemusCoregistration::captureObjectiveCenter()
{
    if (!m_havePenPos) {
        qWarning() << "Objective center capture: no pen position available";
        return false;
    }

    const QMatrix4x4& dev = m_deviceToWorld;
    if (dev.isIdentity()) {
        qWarning() << "Objective center capture: no tracker data available";
        return false;
    }

    // Recover raw tracker pose (remove device offset)
    QMatrix4x4 offsetInv;
    offsetInv.setToIdentity();
    offsetInv.translate(m_offsetTranslation);
    offsetInv.rotate(m_offsetRotation);
    offsetInv = offsetInv.inverted();

    const QMatrix4x4 trackerToWorld = dev * offsetInv;
    const QVector3D trackerPos(trackerToWorld(0, 3), trackerToWorld(1, 3), trackerToWorld(2, 3));
    const QQuaternion trackerOri = QQuaternion::fromRotationMatrix(trackerToWorld.toGenericMatrix<3, 3>());

    // Transform pen position into tracker-local frame
    m_objectiveCenterLocal = trackerOri.inverted().rotatedVector(m_penPosition - trackerPos);
    m_hasObjectiveCenter = true;

    const float distMm = m_objectiveCenterLocal.length() * 1000.0f;
    qInfo().nospace()
        << "Objective center captured (tracker-local): ("
        << m_objectiveCenterLocal.x() * 1000.f << ", "
        << m_objectiveCenterLocal.y() * 1000.f << ", "
        << m_objectiveCenterLocal.z() * 1000.f << ") mm"
        << ", distance from tracker: " << distMm << " mm";

    return true;
}

void PolhemusCoregistration::clearObjectiveCenter()
{
    m_objectiveCenterLocal = QVector3D();
    m_hasObjectiveCenter = false;
}

bool PolhemusCoregistration::solveOpticalCalibration()
{
    const int N = static_cast<int>(m_opticalCalibSamples.size());
    if (N < 2) {
        qWarning() << "Optical calibration: need at least 2 samples, have" << N;
        return false;
    }

    // Step 1: Transform all focus points into the tracker's local frame
    std::vector<Eigen::Vector3d> localPoints(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        const auto& s = m_opticalCalibSamples[static_cast<size_t>(i)];
        const QVector3D local = s.trackerOri.inverted().rotatedVector(s.focusPoint - s.trackerPos);
        localPoints[static_cast<size_t>(i)] = Eigen::Vector3d(local.x(), local.y(), local.z());
    }

    // Step 2: PCA initial estimate — centroid + SVD for starting axis & center
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& p : localPoints) centroid += p;
    centroid /= static_cast<double>(N);

    Eigen::MatrixXd centered(3, N);
    for (int i = 0; i < N; ++i) {
        centered.col(i) = localPoints[static_cast<size_t>(i)] - centroid;
    }

    Eigen::JacobiSVD<Eigen::MatrixXd> svd(centered, Eigen::ComputeThinU);
    Eigen::Vector3d axisDir = svd.matrixU().col(0);
    if (centroid.dot(axisDir) < 0.0) axisDir = -axisDir;

    const double t0 = -centroid.dot(axisDir);
    Eigen::Vector3d opticalCenter = centroid + t0 * axisDir;

    // Step 3: Constrained refinement.
    //
    // Priority: (a) directly captured objective center, (b) known distance
    // constraint, (c) unconstrained PCA fallback.
    //
    // (a) If the user touched the pen to the objective lens, we know the
    //     optical center exactly in tracker-local frame.  Only the axis
    //     direction needs to be determined from the focus samples.
    // (b) If only the distance is known, optimise O on the sphere |O| = R.
    // (c) Otherwise fall back to unconstrained PCA.
    const double knownDist = static_cast<double>(m_knownTrackerToObjectiveDist);
    if (m_hasObjectiveCenter) {
        // Direct objective center — use it as-is, recompute axis from it
        opticalCenter = Eigen::Vector3d(m_objectiveCenterLocal.x(),
                                        m_objectiveCenterLocal.y(),
                                        m_objectiveCenterLocal.z());

        Eigen::MatrixXd rays(3, N);
        for (int i = 0; i < N; ++i)
            rays.col(i) = localPoints[static_cast<size_t>(i)] - opticalCenter;

        Eigen::JacobiSVD<Eigen::MatrixXd> raySvd(rays, Eigen::ComputeThinU);
        axisDir = raySvd.matrixU().col(0);
        // Axis must point from objective toward focus points
        Eigen::Vector3d meanRay = Eigen::Vector3d::Zero();
        for (int i = 0; i < N; ++i) meanRay += rays.col(i);
        if (meanRay.dot(axisDir) < 0.0) axisDir = -axisDir;

        qInfo().nospace()
            << "Optical calibration: using directly captured objective center, |O|="
            << opticalCenter.norm() * 1000.0 << " mm";
    } else if (knownDist > 0.0 && N >= 2) {
        // Project PCA center onto sphere of radius knownDist
        double R = knownDist;
        Eigen::Vector3d O = opticalCenter;
        double Onorm = O.norm();
        if (Onorm > 1e-9) {
            O = O * (R / Onorm);
        } else {
            O = centroid.normalized() * R;
        }

        // Gauss-Newton iterations: optimise O on the sphere, recompute axis each step
        for (int iter = 0; iter < 30; ++iter) {
            // Recompute axis direction from O: SVD of (F_i - O)
            Eigen::MatrixXd rays(3, N);
            for (int i = 0; i < N; ++i)
                rays.col(i) = localPoints[static_cast<size_t>(i)] - O;

            Eigen::JacobiSVD<Eigen::MatrixXd> raySvd(rays, Eigen::ComputeThinU);
            Eigen::Vector3d d = raySvd.matrixU().col(0);
            if (O.dot(d) < 0.0) d = -d;  // axis points away from tracker

            // Compute gradient of cost w.r.t. O (on the tangent plane of the sphere)
            // Cost: E = sum_i |(F_i-O) - ((F_i-O).d)d|^2
            // dE/dO = sum_i -2 * perp_i  where perp_i = (F_i-O) - ((F_i-O).d)d
            // but we project gradient onto tangent plane of sphere at O
            Eigen::Vector3d grad = Eigen::Vector3d::Zero();
            double cost = 0.0;
            for (int i = 0; i < N; ++i) {
                Eigen::Vector3d v = localPoints[static_cast<size_t>(i)] - O;
                Eigen::Vector3d perp = v - v.dot(d) * d;
                cost += perp.squaredNorm();
                grad -= 2.0 * perp;
            }

            // Project gradient onto tangent plane of sphere at O
            Eigen::Vector3d normal = O.normalized();
            Eigen::Vector3d tangentGrad = grad - grad.dot(normal) * normal;

            double gradNorm = tangentGrad.norm();
            if (gradNorm < 1e-12) break;

            // Line search with backtracking
            double step = 0.01 * R / gradNorm;  // conservative initial step
            for (int ls = 0; ls < 10; ++ls) {
                Eigen::Vector3d candidate = O - step * tangentGrad;
                // Project back onto sphere
                candidate = candidate.normalized() * R;

                // Recompute cost at candidate
                Eigen::MatrixXd cRays(3, N);
                for (int i = 0; i < N; ++i)
                    cRays.col(i) = localPoints[static_cast<size_t>(i)] - candidate;
                Eigen::JacobiSVD<Eigen::MatrixXd> cSvd(cRays, Eigen::ComputeThinU);
                Eigen::Vector3d cd = cSvd.matrixU().col(0);

                double cCost = 0.0;
                for (int i = 0; i < N; ++i) {
                    Eigen::Vector3d v = localPoints[static_cast<size_t>(i)] - candidate;
                    Eigen::Vector3d perp = v - v.dot(cd) * cd;
                    cCost += perp.squaredNorm();
                }

                if (cCost < cost) {
                    O = candidate;
                    break;
                }
                step *= 0.5;
            }
        }

        opticalCenter = O;

        // Final axis from refined center
        Eigen::MatrixXd finalRays(3, N);
        for (int i = 0; i < N; ++i)
            finalRays.col(i) = localPoints[static_cast<size_t>(i)] - opticalCenter;
        Eigen::JacobiSVD<Eigen::MatrixXd> finalSvd(finalRays, Eigen::ComputeThinU);
        axisDir = finalSvd.matrixU().col(0);
        if (opticalCenter.dot(axisDir) < 0.0) axisDir = -axisDir;

        qInfo().nospace()
            << "Optical calibration: constrained refinement (R="
            << knownDist * 1000.0 << " mm) converged, |O|="
            << opticalCenter.norm() * 1000.0 << " mm";
    }

    // Step 4: Compute RMS residual (perpendicular distance from each point to the ray from O)
    double sumSq = 0.0;
    for (const auto& p : localPoints) {
        const Eigen::Vector3d diff = p - opticalCenter;
        const double along = diff.dot(axisDir);
        const double perpSq = (diff - along * axisDir).squaredNorm();
        sumSq += perpSq;
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(N)) * 1000.0; // mm

    // Step 5: Depth spread along the optical axis from the refined center
    double minDepth =  1e30, maxDepth = -1e30;
    for (const auto& p : localPoints) {
        const double depth = (p - opticalCenter).dot(axisDir);
        minDepth = std::min(minDepth, depth);
        maxDepth = std::max(maxDepth, depth);
    }
    const double depthSpreadMm = (maxDepth - minDepth) * 1000.0;

    m_opticalAxisLocal   = QVector3D(static_cast<float>(axisDir.x()),
                                     static_cast<float>(axisDir.y()),
                                     static_cast<float>(axisDir.z()));
    m_opticalCenterLocal = QVector3D(static_cast<float>(opticalCenter.x()),
                                     static_cast<float>(opticalCenter.y()),
                                     static_cast<float>(opticalCenter.z()));
    m_opticalCalibResidualMm = static_cast<float>(rms);
    m_opticalCalibDepthSpreadMm = static_cast<float>(depthSpreadMm);
    m_opticalCalibValid = true;

    const float distMm = m_opticalCenterLocal.length() * 1000.0f;

    qInfo().nospace()
        << "Optical calibration solved (" << N << " samples"
        << (knownDist > 0.0 ? ", constrained R=" + QString::number(knownDist * 1000.0, 'f', 1) + " mm" : QString()) << "):"
        << "\n    axis (local):   (" << m_opticalAxisLocal.x() << ", " << m_opticalAxisLocal.y() << ", " << m_opticalAxisLocal.z() << ")"
        << "\n    center (local): (" << m_opticalCenterLocal.x()*1000.f << ", " << m_opticalCenterLocal.y()*1000.f << ", " << m_opticalCenterLocal.z()*1000.f << ") mm"
        << "\n    tracker\u2194center: " << distMm << " mm"
        << "\n    RMS residual:   " << rms << " mm"
        << "\n    depth spread:   " << depthSpreadMm << " mm (range " << minDepth*1000.0 << " .. " << maxDepth*1000.0 << " mm)";

    // Per-sample: report convergence angle between tracker→focus direction
    // and fitted optical axis.  This angle shrinks as the OPMI moves farther
    // from the head (lines become parallel) and grows at close range where
    // the tracker offset dominates.
    qInfo() << "    Per-sample convergence angles (tracker\u2192focus vs optical axis):";
    for (int i = 0; i < N; ++i) {
        const Eigen::Vector3d& p = localPoints[static_cast<size_t>(i)];
        const Eigen::Vector3d trkToFocus = p.normalized();
        const double cosAngle = std::clamp(trkToFocus.dot(axisDir), -1.0, 1.0);
        const double angleDeg = std::acos(cosAngle) * (180.0 / 3.14159265358979);
        const double depth = (p - opticalCenter).dot(axisDir);
        const Eigen::Vector3d diff = p - opticalCenter;
        const double perpDist = (diff - diff.dot(axisDir) * axisDir).norm() * 1000.0;
        qInfo().nospace()
            << "      sample " << (i + 1) << ": depth=" << depth*1000.0
            << " mm, convergence angle=" << angleDeg << "\u00b0"
            << ", perp err=" << perpDist << " mm";
    }

    if (depthSpreadMm < 50.0) {
        qWarning() << "Optical calibration: depth spread is only" << depthSpreadMm
                   << "mm. For a reliable axis direction, move the OPMI to vary the"
                   << "focal distance by at least 50 mm between samples.";
    }

    if (rms > 10.0) {
        qWarning() << "Optical calibration: RMS residual" << rms
                   << "mm is large. Check that the stylus accurately marks the microscope focus point.";
    }

    // Plausibility check: when not using constrained mode, verify the
    // tracker-to-optical-center distance is in a plausible range.
    if (knownDist <= 0.0) {
        constexpr float kMinPlausibleMm   = 100.0f;
        constexpr float kMaxPlausibleMm   = 500.0f;
        if (distMm < kMinPlausibleMm || distMm > kMaxPlausibleMm) {
            qWarning().nospace()
                << "Optical calibration: tracker\u2194axis distance " << distMm
                << " mm is outside the plausible range [" << kMinPlausibleMm
                << ", " << kMaxPlausibleMm << "] mm.  Calibration data may be unreliable.";
        }
    }

    emit opticalCalibrationChanged();
    return true;
}

bool PolhemusCoregistration::opticalRayInWorld(QVector3D& origin, QVector3D& direction) const
{
    if (!m_opticalCalibValid) return false;

    const QMatrix4x4& dev = m_deviceToWorld;
    if (dev.isIdentity()) return false;

    // Recover raw tracker pose (before device offset)
    QMatrix4x4 offsetMat;
    offsetMat.setToIdentity();
    offsetMat.translate(m_offsetTranslation);
    offsetMat.rotate(m_offsetRotation);

    const QMatrix4x4 trackerToWorld = dev * offsetMat.inverted();
    const QVector3D trackerPos(trackerToWorld(0, 3), trackerToWorld(1, 3), trackerToWorld(2, 3));
    const QQuaternion trackerOri = QQuaternion::fromRotationMatrix(trackerToWorld.toGenericMatrix<3, 3>());

    origin    = trackerPos + trackerOri.rotatedVector(m_opticalCenterLocal);
    direction = trackerOri.rotatedVector(m_opticalAxisLocal).normalized();
    return true;
}

//=============================================================================================================

bool PolhemusCoregistration::opticalUpInWorld(QVector3D& up) const
{
    if (!m_opticalCalibValid) return false;

    const QMatrix4x4& dev = m_deviceToWorld;
    if (dev.isIdentity()) return false;

    QMatrix4x4 offsetMat;
    offsetMat.setToIdentity();
    offsetMat.translate(m_offsetTranslation);
    offsetMat.rotate(m_offsetRotation);

    const QMatrix4x4 trackerToWorld = dev * offsetMat.inverted();
    const QQuaternion trackerOri = QQuaternion::fromRotationMatrix(trackerToWorld.toGenericMatrix<3, 3>());

    // Tracker local Z axis = "up" in the microscope view.
    // Orthogonalise against the optical axis to remove any tilt component.
    const QVector3D rawUp = trackerOri.rotatedVector(QVector3D(0.0f, 0.0f, 1.0f));
    const QVector3D axis  = trackerOri.rotatedVector(m_opticalAxisLocal).normalized();
    up = (rawUp - QVector3D::dotProduct(rawUp, axis) * axis).normalized();
    return up.lengthSquared() > 0.5f; // degenerate if Y ≈ optical axis
}

//=============================================================================================================
// Session persistence helpers
//=============================================================================================================

namespace {

void saveVec3(QSettings &s, const QString &key, const QVector3D &v)
{
    s.setValue(key + "/x", static_cast<double>(v.x()));
    s.setValue(key + "/y", static_cast<double>(v.y()));
    s.setValue(key + "/z", static_cast<double>(v.z()));
}

QVector3D loadVec3(const QSettings &s, const QString &key)
{
    return QVector3D(
        static_cast<float>(s.value(key + "/x", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/y", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/z", 0.0).toDouble()));
}

void saveMat4(QSettings &s, const QString &key, const QMatrix4x4 &m)
{
    QByteArray data(reinterpret_cast<const char*>(m.constData()), 16 * sizeof(float));
    s.setValue(key, data);
}

QMatrix4x4 loadMat4(const QSettings &s, const QString &key)
{
    QByteArray data = s.value(key).toByteArray();
    QMatrix4x4 m;
    if (data.size() == 16 * static_cast<int>(sizeof(float)))
        memcpy(m.data(), data.constData(), 16 * sizeof(float));
    return m;
}

void saveQuat(QSettings &s, const QString &key, const QQuaternion &q)
{
    s.setValue(key + "/w", static_cast<double>(q.scalar()));
    s.setValue(key + "/x", static_cast<double>(q.x()));
    s.setValue(key + "/y", static_cast<double>(q.y()));
    s.setValue(key + "/z", static_cast<double>(q.z()));
}

QQuaternion loadQuat(const QSettings &s, const QString &key)
{
    return QQuaternion(
        static_cast<float>(s.value(key + "/w", 1.0).toDouble()),
        static_cast<float>(s.value(key + "/x", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/y", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/z", 0.0).toDouble()));
}

} // anonymous namespace

//=============================================================================================================

void PolhemusCoregistration::saveSessionState(QSettings &settings, const QString &prefix) const
{
    settings.beginGroup(prefix);

    // Stations & axis mirror
    settings.setValue("trackerStation", m_trackerStation);
    settings.setValue("penStation", m_penStation);
    settings.setValue("mirrorX", m_mirrorX);
    settings.setValue("mirrorY", m_mirrorY);

    // Pen fiducials
    const char* fidNames[] = {"LPA", "NAS", "RPA", "CZ"};
    for (int i = 0; i < 4; ++i) {
        settings.setValue(QString("hasPenFid/%1").arg(fidNames[i]), m_hasPenFid[i]);
        if (m_hasPenFid[i])
            saveVec3(settings, QString("penFid/%1").arg(fidNames[i]), m_penFid[i]);
    }

    // Model fiducials
    for (int i = 0; i < 4; ++i) {
        settings.setValue(QString("hasModelFid/%1").arg(fidNames[i]), m_hasModelFid[i]);
        if (m_hasModelFid[i])
            saveVec3(settings, QString("modelFid/%1").arg(fidNames[i]), m_modelFid[i]);
    }

    // Vertex
    settings.setValue("hasPenVertex", m_hasPenVertex);
    if (m_hasPenVertex) saveVec3(settings, "penVertex", m_penVertex);
    settings.setValue("hasModelVertex", m_hasModelVertex);
    if (m_hasModelVertex) saveVec3(settings, "modelVertex", m_modelVertex);

    // Calibration offset
    saveVec3(settings, "offsetTranslation", m_offsetTranslation);
    saveQuat(settings, "offsetRotation", m_offsetRotation);

    // Pen tip offset
    saveVec3(settings, "penTipOffset", m_penTipOffset);
    settings.setValue("tipOffsetEnabled", m_tipOffsetEnabled);

    // Optical calibration
    settings.setValue("opticalCalibValid", m_opticalCalibValid);
    settings.setValue("knownTrackerToObjectiveDist", static_cast<double>(m_knownTrackerToObjectiveDist));
    settings.setValue("hasObjectiveCenter", m_hasObjectiveCenter);
    if (m_hasObjectiveCenter)
        saveVec3(settings, "objectiveCenterLocal", m_objectiveCenterLocal);
    if (m_opticalCalibValid) {
        saveVec3(settings, "opticalAxisLocal", m_opticalAxisLocal);
        saveVec3(settings, "opticalCenterLocal", m_opticalCenterLocal);
        settings.setValue("opticalCalibResidualMm", static_cast<double>(m_opticalCalibResidualMm));
        settings.setValue("opticalCalibDepthSpreadMm", static_cast<double>(m_opticalCalibDepthSpreadMm));
    }

    // Optical calibration samples (so calibration can be re-solved after restart)
    const int nOptSamples = static_cast<int>(m_opticalCalibSamples.size());
    settings.setValue("opticalCalibSampleCount", nOptSamples);
    for (int i = 0; i < nOptSamples; ++i) {
        const auto& s = m_opticalCalibSamples[static_cast<size_t>(i)];
        const QString key = QString("opticalCalibSample/%1").arg(i);
        saveVec3(settings, key + "/trackerPos", s.trackerPos);
        saveQuat(settings, key + "/trackerOri", s.trackerOri);
        saveVec3(settings, key + "/focusPoint", s.focusPoint);
    }

    // Registration transforms
    settings.setValue("registrationValid", m_registrationValid);
    if (m_registrationValid) {
        saveMat4(settings, "headToDevice", m_headToDevice);
        saveMat4(settings, "headToWorld", m_headToWorld);
        saveMat4(settings, "worldToModel", m_worldToModel);
    }

    settings.endGroup();
}

//=============================================================================================================

bool PolhemusCoregistration::restoreSessionState(QSettings &settings, const QString &prefix)
{
    settings.beginGroup(prefix);
    if (settings.allKeys().isEmpty()) {
        settings.endGroup();
        return false;
    }

    // Stations & axis mirror
    m_trackerStation = settings.value("trackerStation", 1).toInt();
    m_penStation     = settings.value("penStation", 2).toInt();
    m_mirrorX        = settings.value("mirrorX", false).toBool();
    m_mirrorY        = settings.value("mirrorY", false).toBool();

    // Pen fiducials
    const char* fidNames[] = {"LPA", "NAS", "RPA", "CZ"};
    for (int i = 0; i < 4; ++i) {
        m_hasPenFid[i] = settings.value(QString("hasPenFid/%1").arg(fidNames[i]), false).toBool();
        if (m_hasPenFid[i])
            m_penFid[i] = loadVec3(settings, QString("penFid/%1").arg(fidNames[i]));
    }

    // Model fiducials
    for (int i = 0; i < 4; ++i) {
        m_hasModelFid[i] = settings.value(QString("hasModelFid/%1").arg(fidNames[i]), false).toBool();
        if (m_hasModelFid[i])
            m_modelFid[i] = loadVec3(settings, QString("modelFid/%1").arg(fidNames[i]));
    }

    // Vertex
    m_hasPenVertex = settings.value("hasPenVertex", false).toBool();
    if (m_hasPenVertex) m_penVertex = loadVec3(settings, "penVertex");
    m_hasModelVertex = settings.value("hasModelVertex", false).toBool();
    if (m_hasModelVertex) m_modelVertex = loadVec3(settings, "modelVertex");

    // Calibration offset
    m_offsetTranslation = loadVec3(settings, "offsetTranslation");
    m_offsetRotation    = loadQuat(settings, "offsetRotation");

    // Pen tip offset
    m_penTipOffset      = loadVec3(settings, "penTipOffset");
    m_tipOffsetEnabled  = settings.value("tipOffsetEnabled", false).toBool();

    // Optical calibration
    m_knownTrackerToObjectiveDist = static_cast<float>(settings.value("knownTrackerToObjectiveDist", 0.200).toDouble());
    m_hasObjectiveCenter = settings.value("hasObjectiveCenter", false).toBool();
    if (m_hasObjectiveCenter)
        m_objectiveCenterLocal = loadVec3(settings, "objectiveCenterLocal");
    m_opticalCalibValid = settings.value("opticalCalibValid", false).toBool();
    if (m_opticalCalibValid) {
        m_opticalAxisLocal         = loadVec3(settings, "opticalAxisLocal");
        m_opticalCenterLocal       = loadVec3(settings, "opticalCenterLocal");
        m_opticalCalibResidualMm   = static_cast<float>(settings.value("opticalCalibResidualMm", 0.0).toDouble());
        m_opticalCalibDepthSpreadMm = static_cast<float>(settings.value("opticalCalibDepthSpreadMm", 0.0).toDouble());
    }

    // Optical calibration samples
    const int nOptSamples = settings.value("opticalCalibSampleCount", 0).toInt();
    m_opticalCalibSamples.clear();
    m_opticalCalibSamples.reserve(static_cast<size_t>(nOptSamples));
    for (int i = 0; i < nOptSamples; ++i) {
        const QString key = QString("opticalCalibSample/%1").arg(i);
        OpticalCalibSample s;
        s.trackerPos = loadVec3(settings, key + "/trackerPos");
        s.trackerOri = loadQuat(settings, key + "/trackerOri");
        s.focusPoint = loadVec3(settings, key + "/focusPoint");
        m_opticalCalibSamples.push_back(s);
    }

    // Registration transforms
    m_registrationValid = settings.value("registrationValid", false).toBool();
    if (m_registrationValid) {
        m_headToDevice = loadMat4(settings, "headToDevice");
        m_headToWorld  = loadMat4(settings, "headToWorld");
        m_worldToModel = loadMat4(settings, "worldToModel");
    }

    settings.endGroup();

    // Re-populate the AcquiredPoints list so that countOf() / UI status
    // reflect the restored fiducials.
    if (m_pPoints) {
        const char* fidLabels[] = { "", "LPA", "NAS", "RPA" };
        for (int i = 1; i <= 3; ++i) {
            if (m_hasPenFid[i]) {
                DigitizedPoint dp;
                dp.kind        = PointKind::Fiducial;
                dp.label       = QString::fromLatin1(fidLabels[i]);
                dp.identNumber = i;
                dp.position    = m_penFid[i];
                m_pPoints->append(dp);
            }
        }
    }

    if (m_registrationValid)
        emit registrationChanged();

    return m_registrationValid;
}
