//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::PolhemusCoregistration — fiducial-based head→device rigid alignment.
 *
 * The three cardinal fiducials (LPA, NAS, RPA) define an
 * orthonormal head frame whose construction matches the
 * MEG/EEG convention used throughout mne-cpp. The head→device
 * transform is obtained by composing that frame with the pre-
 * calibrated tracker-to-device offset and with the live tracker
 * pose streamed from the Polhemus connection, all as Eigen
 * 4x4 matrices for the viewer and the navigation export.
 */

#include "polhemus_coregistration.h"
#include <base/log.h>

#include <format>
#include "acquired_points.h"

#include <Eigen/Dense>
#include <QSettings>
#include <cmath>
#include <cstring>

using namespace mne;

//=============================================================================================================

namespace
{

Eigen::Matrix4f poseMatrix(const Eigen::Vector3f& translation, const Eigen::Quaternionf& rotation)
{
    Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();
    pose.topLeftCorner<3, 3>() = rotation.toRotationMatrix();
    pose.topRightCorner<3, 1>() = translation;
    return pose;
}

// Raw tracker pose: deviceToWorld with the tracker-to-device offset removed.
void trackerPose(const Eigen::Matrix4f& deviceToWorld,
                 const Eigen::Vector3f& offsetTranslation,
                 const Eigen::Quaternionf& offsetRotation,
                 Eigen::Vector3f& position,
                 Eigen::Quaternionf& orientation)
{
    const Eigen::Matrix4f trackerToWorld = deviceToWorld * poseMatrix(offsetTranslation, offsetRotation).inverse();
    position = trackerToWorld.topRightCorner<3, 1>();
    orientation = Eigen::Quaternionf(Eigen::Matrix3f(trackerToWorld.topLeftCorner<3, 3>()));
}

bool isIdentity(const Eigen::Matrix4f& m)
{
    return m == Eigen::Matrix4f::Identity();
}

} // anonymous namespace

//=============================================================================================================

PolhemusCoregistration::PolhemusCoregistration(QObject* parent)
: QObject(parent)
, m_pPoints(new AcquiredPoints(this))
{
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

void PolhemusCoregistration::setProbeStation(int station)
{
    m_probeStation = station;
}

//=============================================================================================================

void PolhemusCoregistration::setTrackerToDeviceOffset(const Eigen::Vector3f& translation,
                                                      const Eigen::Quaternionf& rotation)
{
    m_offsetTranslation = translation;
    m_offsetRotation = rotation;
}

//=============================================================================================================

bool PolhemusCoregistration::captureCurrentPenPositionAsFiducial(FiducialId id)
{
    if (!m_havePenPos) {
        return false;
    }

    static const char* labels[] = {nullptr, "LPA", "NAS", "RPA"};
    const int ident = static_cast<int>(id);

    // Store pen fiducial BEFORE append (which emits pointsChanged)
    m_penFid[ident] = m_penPosition;
    m_hasPenFid[ident] = true;

    mne::Log::info("Fiducial captured: {}  ({}, {}, {}) mm", labels[ident], m_penPosition.x() * 1000.f, m_penPosition.y() * 1000.f, m_penPosition.z() * 1000.f);

    // Log distances to previously captured fiducials
    for (int j = 1; j <= 3; ++j) {
        if (j != ident && m_hasPenFid[j]) {
            float dist = (m_penFid[ident] - m_penFid[j]).norm() * 1000.0f;
            mne::Log::info("  {} ↔ {}: {} mm{}", labels[ident], labels[j], dist, (dist > 200.0f ? "  *** WARNING: > 200 mm!" : ""));
        }
    }

    m_pPoints->removeFiducial(id);

    DigitizedPoint dp;
    dp.kind = PointKind::Fiducial;
    dp.label = QString::fromLatin1(labels[ident]);
    dp.identNumber = ident;
    dp.position = m_penPosition;
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
    dp.kind = PointKind::HeadShape;
    dp.label = QStringLiteral("HSP-%1").arg(n);
    dp.identNumber = n;
    dp.position = m_penPosition;
    m_pPoints->append(dp);
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::resetRegistration()
{
    m_registrationValid = false;
    m_headToWorld.setIdentity();
    m_headToDevice.setIdentity();
    m_worldToModel.setIdentity();
    m_hasPenVertex = false;
    for (int i = 0; i < 4; ++i)
        m_hasPenFid[i] = false;
    emit registrationChanged();
}

//=============================================================================================================

void PolhemusCoregistration::setModelFiducial(FiducialId id, const Eigen::Vector3f& posInModel)
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

Eigen::Vector3f PolhemusCoregistration::modelFiducial(FiducialId id) const
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
    if (!m_havePenPos)
        return false;
    m_penVertex = m_penPosition;
    m_hasPenVertex = true;
    mne::Log::info("Captured pen vertex (CZ) at {} {} {} mm", m_penVertex.x() * 1000.0f, m_penVertex.y() * 1000.0f, m_penVertex.z() * 1000.0f);
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
    const Eigen::Vector3f pNas = m_penFid[static_cast<int>(FiducialId::NAS)];
    const Eigen::Vector3f pLpa = m_penFid[static_cast<int>(FiducialId::LPA)];
    const Eigen::Vector3f pRpa = m_penFid[static_cast<int>(FiducialId::RPA)];

    // Spread check: reject degenerate input
    const float dNL = (pNas - pLpa).norm() * 1000.0f;
    const float dNR = (pNas - pRpa).norm() * 1000.0f;
    const float dLR = (pLpa - pRpa).norm() * 1000.0f;
    const float minSpread = std::min({dNL, dNR, dLR});
    if (minSpread < 20.0f) {
        mne::Log::warning("Registration failed: pen fiducials too close (min spread {} mm, need > 20 mm)", minSpread);
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
        const Eigen::Vector3f mNas = m_modelFid[static_cast<int>(FiducialId::NAS)];
        const Eigen::Vector3f mLpa = m_modelFid[static_cast<int>(FiducialId::LPA)];
        const Eigen::Vector3f mRpa = m_modelFid[static_cast<int>(FiducialId::RPA)];

        // Compare pen vs model fiducial distances — should be similar for the same head
        const float mNL = (mNas - mLpa).norm() * 1000.0f;
        const float mNR = (mNas - mRpa).norm() * 1000.0f;
        const float mLR = (mLpa - mRpa).norm() * 1000.0f;

        const float maxRatio = std::max({dNL / mNL, dNR / mNR, dLR / mLR});
        const float minRatio = std::min({dNL / mNL, dNR / mNR, dLR / mLR});
        if (maxRatio / minRatio > 2.0f) {
            mne::Log::warning("Registration: shape mismatch — pen fiducial triangle has very different proportions from model. Check fiducial placement!");
        }

        // --- Kabsch algorithm (SVD least-squares rigid alignment) ---

        // 1. Centroids
        const Eigen::Vector3d penC = (pNas + pLpa + pRpa).cast<double>() / 3.0;
        const Eigen::Vector3d modC = (mNas + mLpa + mRpa).cast<double>() / 3.0;

        // 2. Centered point matrices (3×3, each column = one centered point)
        Eigen::Matrix3d P, Q;
        P.col(0) = pNas.cast<double>() - penC;
        P.col(1) = pLpa.cast<double>() - penC;
        P.col(2) = pRpa.cast<double>() - penC;

        Q.col(0) = mNas.cast<double>() - modC;
        Q.col(1) = mLpa.cast<double>() - modC;
        Q.col(2) = mRpa.cast<double>() - modC;

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
            const Eigen::Vector3d penCZ = m_penVertex.cast<double>();
            const Eigen::Vector3d modCZ = m_modelVertex.cast<double>();

            const Eigen::Vector3d mappedCZ = R * penCZ + t;
            const double errCurrent = (mappedCZ - modCZ).norm();

            // Try the alternative sign
            Eigen::Matrix3d D_alt = D;
            D_alt(2, 2) = -D(2, 2);
            const Eigen::Matrix3d R_alt = V * D_alt * U.transpose();
            const Eigen::Vector3d t_alt = modC - R_alt * penC;
            const Eigen::Vector3d mappedCZ_alt = R_alt * penCZ + t_alt;
            const double errAlt = (mappedCZ_alt - modCZ).norm();

            if (errAlt < errCurrent) {
                R = R_alt;
                t = t_alt;
                D = D_alt;
            }
        }

        // 7. worldToModel = [R | t]
        m_worldToModel.setIdentity();
        m_worldToModel.topLeftCorner<3, 3>() = R.cast<float>();
        m_worldToModel.topRightCorner<3, 1>() = t.cast<float>();

        m_headToWorld.setIdentity();
        m_headToDevice = m_deviceToWorld.inverse() * m_headToWorld;
        m_registrationValid = true;
        mne::Log::info("Registration succeeded (analytical paired).");
        emit registrationChanged();
        return true;
    }

    // --- Fallback: head-frame-only registration ---
    const Eigen::Matrix4f headFrame = buildHeadFrame();
    if (headFrame.block<3, 1>(0, 2).norm() < 1e-6f) {
        return false;
    }

    m_headToWorld = headFrame;
    m_headToDevice = m_deviceToWorld.inverse() * m_headToWorld;
    m_worldToModel.setIdentity();
    m_registrationValid = true;
    mne::Log::info("Registration succeeded (head-frame fallback).");
    emit registrationChanged();
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::on_pose(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)
{
    // Apply axis mirroring to compensate for transmitter placement
    const Eigen::Vector3f pos(m_mirrorX ? -position.x() : position.x(),
                              m_mirrorY ? -position.y() : position.y(),
                              position.z());

    if (station == m_trackerStation) {
        m_deviceToWorld = buildDevicePose(pos, orientation);
        emit devicePoseChanged(m_deviceToWorld);
    } else if (station == m_penStation) {
        // Gimbal-lock guard: for ZYX Euler, sin(el) = 2*(w*y - x*z).
        // When |el| > 80° the Euler→quaternion conversion is unreliable,
        // so freeze the pen pose at its last good value.
        const float sinEl = 2.0f * (orientation.w() * orientation.y() - orientation.x() * orientation.z());
        constexpr float kGimbalSinEl = 0.9848f; // sin(80°)
        const bool gimbalLock = (std::abs(sinEl) > kGimbalSinEl);

        if (!gimbalLock) {
            const Eigen::Vector3f tipAdj = m_tipOffsetEnabled
                ? Eigen::Vector3f(orientation * m_penTipOffset)
                : Eigen::Vector3f::Zero();
            m_penPosition = pos + tipAdj;
            m_penOrientation = orientation;
            m_havePenPos = true;
        }

        // Collect raw (un-offset) samples during pivot calibration.
        // Only keep samples with sufficient angular change from the last
        // accepted sample to avoid redundant near-identical rows in the SVD.
        // Also reject position jumps and gimbal-lock orientations.
        if (m_pivotState == PivotState::Collecting) {
            constexpr float kMinAngleDeg = 3.0f;
            constexpr float kMaxPosJumpM = 0.05f;    // 5 cm
            constexpr float kGimbalSinEl2 = 0.9848f; // sin(80°) — reject |el|>80°

            // Gimbal-lock check: for ZYX Euler, sin(el) = 2*(w*y - x*z)
            const float sinEl2 = 2.0f * (orientation.w() * orientation.y() - orientation.x() * orientation.z());
            if (std::abs(sinEl2) > kGimbalSinEl2) {
                // Near gimbal lock — skip this sample silently
            } else {
                bool accept = m_pivotOrientations.empty();
                if (!accept) {
                    const Eigen::Quaternionf& prev = m_pivotOrientations.back();
                    float dot = std::abs(prev.dot(orientation));
                    float angleDeg = 2.0f * std::acos(std::min(dot, 1.0f)) * (180.0f / 3.14159265f);
                    float posDelta = (pos - m_pivotPositions.back()).norm();
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
                            float d = std::abs(first.dot(m_pivotOrientations[k]));
                            if (d < minDot)
                                minDot = d;
                        }
                        spanDeg = 2.0f * std::acos(std::min(minDot, 1.0f)) * (180.0f / 3.14159265f);
                    }
                    emit pivotSampleCollected(static_cast<int>(m_pivotPositions.size()), spanDeg);
                }
            }
        }

        emit penPoseChanged(m_penPosition, m_penOrientation);
    } else if (station == m_probeStation) {
        m_probePosition = pos;
        m_probeOrientation = orientation;
        m_haveProbePos = true;
        emit probePoseChanged(m_probePosition, m_probeOrientation);
    }
}

void PolhemusCoregistration::on_button(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)
{
    if (station == m_penStation) {
        // Apply axis mirroring to compensate for transmitter placement
        const Eigen::Vector3f pos(m_mirrorX ? -position.x() : position.x(),
                                  m_mirrorY ? -position.y() : position.y(),
                                  position.z());

        const Eigen::Vector3f tipAdj = m_tipOffsetEnabled
            ? Eigen::Vector3f(orientation * m_penTipOffset)
            : Eigen::Vector3f::Zero();
        m_penPosition = pos + tipAdj;
        m_penOrientation = orientation;
        m_havePenPos = true;

        // Pivot calibration state machine
        if (m_pivotState == PivotState::WaitingForStart) {
            m_pivotState = PivotState::Collecting;
            m_pivotPositions.clear();
            m_pivotOrientations.clear();
            mne::Log::info("Pivot calibration: collecting — pivot the pen around its tip");
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

Eigen::Matrix4f PolhemusCoregistration::buildDevicePose(const Eigen::Vector3f& trackerPos,
                                                        const Eigen::Quaternionf& trackerOri) const
{
    return poseMatrix(trackerPos, trackerOri) * poseMatrix(m_offsetTranslation, m_offsetRotation);
}

Eigen::Matrix4f PolhemusCoregistration::buildHeadFrame() const
{
    const Eigen::Vector3f nas = m_pPoints->fiducial(FiducialId::NAS);
    const Eigen::Vector3f lpa = m_pPoints->fiducial(FiducialId::LPA);
    const Eigen::Vector3f rpa = m_pPoints->fiducial(FiducialId::RPA);

    const Eigen::Vector3f origin = (lpa + rpa) * 0.5f;
    const Eigen::Vector3f ex = (nas - origin).normalized();
    const Eigen::Vector3f eyApprox = (lpa - origin).normalized();
    // Gram-Schmidt: orthogonalize ey against ex, preserving LPA direction
    const Eigen::Vector3f ey = (eyApprox - eyApprox.dot(ex) * ex).normalized();
    const Eigen::Vector3f ez = ex.cross(ey).normalized();

    Eigen::Matrix4f frame = Eigen::Matrix4f::Identity();
    frame.block<3, 1>(0, 0) = ex;
    frame.block<3, 1>(0, 1) = ey;
    frame.block<3, 1>(0, 2) = ez;
    frame.block<3, 1>(0, 3) = origin;
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
    mne::Log::info("Pivot calibration: press stylus button to begin collecting");
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
        mne::Log::warning("Pivot calibration: only {} samples, need at least 10", N);
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
        const Eigen::Vector3f& p = m_pivotPositions[static_cast<size_t>(i)];
        const Eigen::Quaternionf& q = m_pivotOrientations[static_cast<size_t>(i)];

        const Eigen::Matrix3f rm = q.toRotationMatrix();

        const int row = 3 * i;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                A(row + r, c) = static_cast<double>(rm(r, c)); // R_i
                A(row + r, 3 + c) = (r == c) ? -1.0 : 0.0;     // -I
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
        mne::Log::warning("Pivot calibration: ill-conditioned (cond = {} ). Pivot the pen through a wider range of angles.", cond);
        m_pivotState = PivotState::Idle;
        emit pivotStateChanged(m_pivotState);
        return false;
    }

    Eigen::VectorXd x = svd.solve(b);

    const Eigen::Vector3f offset = x.head<3>().cast<float>();
    const Eigen::Vector3f tipPos = x.tail<3>().cast<float>();

    // Compute RMS residual
    double sumSq = 0.0;
    for (int i = 0; i < N; ++i) {
        const Eigen::Vector3f& p = m_pivotPositions[static_cast<size_t>(i)];
        const Eigen::Quaternionf& q = m_pivotOrientations[static_cast<size_t>(i)];
        const Eigen::Vector3f computed = p + q * offset;
        float err = (computed - tipPos).norm();
        sumSq += static_cast<double>(err) * static_cast<double>(err);
    }
    float rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(N))) * 1000.0f;
    m_pivotResidualMm = rms;

    mne::Log::info("Pivot calibration: {} samples, cond = {} , offset = {} {} {} mm, RMS residual = {} mm", N, cond, offset.x() * 1000.0f, offset.y() * 1000.0f, offset.z() * 1000.0f, rms);

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
        mne::Log::warning("Optical calibration: no pen position available");
        return false;
    }

    const Eigen::Matrix4f& dev = m_deviceToWorld;
    if (isIdentity(dev)) {
        mne::Log::warning("Optical calibration: no tracker data available");
        return false;
    }

    // Extract tracker position and orientation from deviceToWorld.
    // deviceToWorld = trackerToWorld * offset, but we want the raw tracker
    // pose (before offset). Reconstruct from the current raw tracker data
    // by removing the offset: trackerToWorld = deviceToWorld * offset^-1
    Eigen::Vector3f trackerPos;
    Eigen::Quaternionf trackerOri;
    trackerPose(dev, m_offsetTranslation, m_offsetRotation, trackerPos, trackerOri);

    OpticalCalibSample sample;
    sample.trackerPos = trackerPos;
    sample.trackerOri = trackerOri;
    sample.focusPoint = m_penPosition;
    m_opticalCalibSamples.push_back(sample);

    const int n = static_cast<int>(m_opticalCalibSamples.size());
    const Eigen::Vector3f localFocus = trackerOri.inverse() * (m_penPosition - trackerPos);
    const float distToTracker = localFocus.norm() * 1000.0f;

    mne::Log::info("Optical calibration sample {}:\n    tracker pos: ({}, {}, {}) mm\n    focus point: ({}, {}, {}) mm\n    focus (local): ({}, {}, {}) mm\n    distance tracker\u2194focus: {} mm", n, trackerPos.x() * 1000.f, trackerPos.y() * 1000.f, trackerPos.z() * 1000.f, m_penPosition.x() * 1000.f, m_penPosition.y() * 1000.f, m_penPosition.z() * 1000.f, localFocus.x() * 1000.f, localFocus.y() * 1000.f, localFocus.z() * 1000.f, distToTracker);

    // Log convergence angle between tracker→focus directions for successive samples.
    // As the OPMI moves farther away, this angle shrinks (lines become parallel);
    // at close range the angle is large because the ~20 cm tracker offset dominates.
    if (n >= 2) {
        const auto& prev = m_opticalCalibSamples[static_cast<size_t>(n - 2)];
        const Eigen::Vector3f prevLocal = prev.trackerOri.inverse() * (prev.focusPoint - prev.trackerPos);
        const Eigen::Vector3f curDir = localFocus.normalized();
        const Eigen::Vector3f prevDir = prevLocal.normalized();
        const float dot = std::clamp(curDir.dot(prevDir), -1.0f, 1.0f);
        const float angleDeg = std::acos(dot) * (180.0f / 3.14159265f);
        const float prevDist = prevLocal.norm() * 1000.0f;
        mne::Log::info("    convergence angle (sample {}\u2194{}): {}\u00b0  (distances: {} / {} mm)", n - 1, n, angleDeg, prevDist, distToTracker);
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
        mne::Log::warning("Objective center capture: no pen position available");
        return false;
    }

    const Eigen::Matrix4f& dev = m_deviceToWorld;
    if (isIdentity(dev)) {
        mne::Log::warning("Objective center capture: no tracker data available");
        return false;
    }

    // Recover raw tracker pose (remove device offset)
    Eigen::Vector3f trackerPos;
    Eigen::Quaternionf trackerOri;
    trackerPose(dev, m_offsetTranslation, m_offsetRotation, trackerPos, trackerOri);

    // Transform pen position into tracker-local frame
    m_objectiveCenterLocal = trackerOri.inverse() * (m_penPosition - trackerPos);
    m_hasObjectiveCenter = true;

    const float distMm = m_objectiveCenterLocal.norm() * 1000.0f;
    mne::Log::info("Objective center captured (tracker-local): ({}, {}, {}) mm, distance from tracker: {} mm", m_objectiveCenterLocal.x() * 1000.f, m_objectiveCenterLocal.y() * 1000.f, m_objectiveCenterLocal.z() * 1000.f, distMm);

    return true;
}

void PolhemusCoregistration::clearObjectiveCenter()
{
    m_objectiveCenterLocal = Eigen::Vector3f::Zero();
    m_hasObjectiveCenter = false;
}

bool PolhemusCoregistration::solveOpticalCalibration()
{
    const int N = static_cast<int>(m_opticalCalibSamples.size());
    if (N < 2) {
        mne::Log::warning("Optical calibration: need at least 2 samples, have {}", N);
        return false;
    }

    // Step 1: Transform all focus points into the tracker's local frame
    std::vector<Eigen::Vector3d> localPoints(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        const auto& s = m_opticalCalibSamples[static_cast<size_t>(i)];
        const Eigen::Vector3f local = s.trackerOri.inverse() * (s.focusPoint - s.trackerPos);
        localPoints[static_cast<size_t>(i)] = local.cast<double>();
    }

    // Step 2: PCA initial estimate — centroid + SVD for starting axis & center
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& p : localPoints)
        centroid += p;
    centroid /= static_cast<double>(N);

    Eigen::MatrixXd centered(3, N);
    for (int i = 0; i < N; ++i) {
        centered.col(i) = localPoints[static_cast<size_t>(i)] - centroid;
    }

    Eigen::JacobiSVD<Eigen::MatrixXd> svd(centered, Eigen::ComputeThinU);
    Eigen::Vector3d axisDir = svd.matrixU().col(0);
    if (centroid.dot(axisDir) < 0.0)
        axisDir = -axisDir;

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
        opticalCenter = m_objectiveCenterLocal.cast<double>();

        Eigen::MatrixXd rays(3, N);
        for (int i = 0; i < N; ++i)
            rays.col(i) = localPoints[static_cast<size_t>(i)] - opticalCenter;

        Eigen::JacobiSVD<Eigen::MatrixXd> raySvd(rays, Eigen::ComputeThinU);
        axisDir = raySvd.matrixU().col(0);
        // Axis must point from objective toward focus points
        Eigen::Vector3d meanRay = Eigen::Vector3d::Zero();
        for (int i = 0; i < N; ++i)
            meanRay += rays.col(i);
        if (meanRay.dot(axisDir) < 0.0)
            axisDir = -axisDir;

        mne::Log::info("Optical calibration: using directly captured objective center, |O|={} mm", opticalCenter.norm() * 1000.0);
    } else if (knownDist > 0.0 && N >= 2) {
        // Start where the PCA axis meets the sphere |O| = R on the side the axis points away from the
        // tracker; the descent below only takes small steps, so it cannot travel from a distant start.
        double R = knownDist;
        Eigen::Vector3d O = opticalCenter;
        const double along = centroid.dot(axisDir);
        const double disc = along * along - centroid.squaredNorm() + R * R;
        if (disc >= 0.0) {
            O = centroid + (-along + std::sqrt(disc)) * axisDir;
        } else if (O.norm() > 1e-9) {
            O = O * (R / O.norm());
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
            if (O.dot(d) < 0.0)
                d = -d; // axis points away from tracker

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
            if (gradNorm < 1e-12)
                break;

            // Line search with backtracking
            double step = 0.01 * R / gradNorm; // conservative initial step
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
        if (opticalCenter.dot(axisDir) < 0.0)
            axisDir = -axisDir;

        mne::Log::info("Optical calibration: constrained refinement (R={} mm) converged, |O|={} mm", knownDist * 1000.0, opticalCenter.norm() * 1000.0);
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
    double minDepth = 1e30, maxDepth = -1e30;
    for (const auto& p : localPoints) {
        const double depth = (p - opticalCenter).dot(axisDir);
        minDepth = std::min(minDepth, depth);
        maxDepth = std::max(maxDepth, depth);
    }
    const double depthSpreadMm = (maxDepth - minDepth) * 1000.0;

    m_opticalAxisLocal = axisDir.cast<float>();
    m_opticalCenterLocal = opticalCenter.cast<float>();
    m_opticalCalibResidualMm = static_cast<float>(rms);
    m_opticalCalibDepthSpreadMm = static_cast<float>(depthSpreadMm);
    m_opticalCalibValid = true;

    const float distMm = m_opticalCenterLocal.norm() * 1000.0f;

    const std::string constraint = knownDist > 0.0 ? std::format(", constrained R={:.1f} mm", knownDist * 1000.0) : std::string();
    mne::Log::info("Optical calibration solved ({} samples{}):\n"
                   "    axis (local):   ({:.4f}, {:.4f}, {:.4f})\n"
                   "    center (local): ({:.1f}, {:.1f}, {:.1f}) mm\n"
                   "    tracker\u2194center: {:.1f} mm\n"
                   "    RMS residual:   {:.2f} mm\n"
                   "    depth spread:   {:.1f} mm (range {:.0f} .. {:.0f} mm)",
                   N, constraint,
                   m_opticalAxisLocal.x(), m_opticalAxisLocal.y(), m_opticalAxisLocal.z(),
                   m_opticalCenterLocal.x() * 1000.f, m_opticalCenterLocal.y() * 1000.f, m_opticalCenterLocal.z() * 1000.f,
                   distMm, rms, depthSpreadMm, minDepth * 1000.0, maxDepth * 1000.0);

    // Per-sample: report convergence angle between tracker→focus direction
    // and fitted optical axis.  This angle shrinks as the OPMI moves farther
    // from the head (lines become parallel) and grows at close range where
    // the tracker offset dominates.
    mne::Log::info("    Per-sample convergence angles (tracker\u2192focus vs optical axis):");
    for (int i = 0; i < N; ++i) {
        const Eigen::Vector3d& p = localPoints[static_cast<size_t>(i)];
        const Eigen::Vector3d trkToFocus = p.normalized();
        const double cosAngle = std::clamp(trkToFocus.dot(axisDir), -1.0, 1.0);
        const double angleDeg = std::acos(cosAngle) * (180.0 / 3.14159265358979);
        const double depth = (p - opticalCenter).dot(axisDir);
        const Eigen::Vector3d diff = p - opticalCenter;
        const double perpDist = (diff - diff.dot(axisDir) * axisDir).norm() * 1000.0;
        mne::Log::info("      sample {}: depth={} mm, convergence angle={}\u00b0, perp err={} mm", (i + 1), depth * 1000.0, angleDeg, perpDist);
    }

    if (depthSpreadMm < 50.0) {
        mne::Log::warning("Optical calibration: depth spread is only {} mm. For a reliable axis direction, move the OPMI to vary the focal distance by at least 50 mm between samples.", depthSpreadMm);
    }

    if (rms > 10.0) {
        mne::Log::warning("Optical calibration: RMS residual {} mm is large. Check that the stylus accurately marks the microscope focus point.", rms);
    }

    // Plausibility check: when not using constrained mode, verify the
    // tracker-to-optical-center distance is in a plausible range.
    if (knownDist <= 0.0) {
        constexpr float kMinPlausibleMm = 100.0f;
        constexpr float kMaxPlausibleMm = 500.0f;
        if (distMm < kMinPlausibleMm || distMm > kMaxPlausibleMm) {
            mne::Log::warning("Optical calibration: tracker\u2194axis distance {} mm is outside the plausible range [{}, {}] mm.  Calibration data may be unreliable.", distMm, kMinPlausibleMm, kMaxPlausibleMm);
        }
    }

    emit opticalCalibrationChanged();
    return true;
}

bool PolhemusCoregistration::opticalRayInWorld(Eigen::Vector3f& origin, Eigen::Vector3f& direction) const
{
    if (!m_opticalCalibValid)
        return false;

    const Eigen::Matrix4f& dev = m_deviceToWorld;
    if (isIdentity(dev))
        return false;

    // Recover raw tracker pose (before device offset)
    Eigen::Vector3f trackerPos;
    Eigen::Quaternionf trackerOri;
    trackerPose(dev, m_offsetTranslation, m_offsetRotation, trackerPos, trackerOri);

    origin = trackerPos + trackerOri * m_opticalCenterLocal;
    direction = (trackerOri * m_opticalAxisLocal).normalized();
    return true;
}

//=============================================================================================================

bool PolhemusCoregistration::opticalUpInWorld(Eigen::Vector3f& up) const
{
    if (!m_opticalCalibValid)
        return false;

    const Eigen::Matrix4f& dev = m_deviceToWorld;
    if (isIdentity(dev))
        return false;

    Eigen::Vector3f trackerPos;
    Eigen::Quaternionf trackerOri;
    trackerPose(dev, m_offsetTranslation, m_offsetRotation, trackerPos, trackerOri);

    // Tracker local Z axis = "up" in the microscope view.
    // Orthogonalise against the optical axis to remove any tilt component.
    const Eigen::Vector3f rawUp = trackerOri * Eigen::Vector3f::UnitZ();
    const Eigen::Vector3f axis = (trackerOri * m_opticalAxisLocal).normalized();
    up = (rawUp - rawUp.dot(axis) * axis).normalized();
    return up.squaredNorm() > 0.5f; // degenerate if Y ≈ optical axis
}

//=============================================================================================================

bool PolhemusCoregistration::applyOpticalAxisFineAdjust(const Eigen::Vector3f& targetWorldPos,
                                                        float& correctionDeg)
{
    correctionDeg = 0.0f;
    if (!m_opticalCalibValid)
        return false;

    // Get tracker-to-world transform (same as opticalRayInWorld)
    const Eigen::Matrix4f& dev = m_deviceToWorld;
    if (isIdentity(dev))
        return false;

    Eigen::Vector3f trackerPos;
    Eigen::Quaternionf trackerOri;
    trackerPose(dev, m_offsetTranslation, m_offsetRotation, trackerPos, trackerOri);

    // Current optical center in world frame
    const Eigen::Vector3f optCenterWorld = trackerPos + trackerOri * m_opticalCenterLocal;

    // Desired axis direction: from optical center to the target point
    const Eigen::Vector3f toTarget = (targetWorldPos - optCenterWorld);
    if (toTarget.norm() < 0.001f)
        return false; // target too close to optical center

    const Eigen::Vector3f desiredDirWorld = toTarget.normalized();

    // Current axis direction in world frame
    const Eigen::Vector3f currentDirWorld = (trackerOri * m_opticalAxisLocal).normalized();

    // Compute correction angle
    const float cosAngle = std::clamp(currentDirWorld.dot(desiredDirWorld), -1.0f, 1.0f);
    correctionDeg = std::acos(cosAngle) * (180.0f / 3.14159265358979f);

    // Sanity: reject corrections > 10° — likely a user error
    if (correctionDeg > 10.0f) {
        mne::Log::warning("Optical fine adjust: correction {}° exceeds 10° limit — rejected.", correctionDeg);
        return false;
    }

    // Save pre-adjustment axis for undo
    if (!m_opticalFineAdjustApplied)
        m_opticalAxisPreFineAdjust = m_opticalAxisLocal;

    // Compute the rotation from current to desired direction, in world frame
    const Eigen::Quaternionf worldCorrection = Eigen::Quaternionf::FromTwoVectors(currentDirWorld, desiredDirWorld);

    // Transform the correction into tracker-local frame:
    // localCorrection = trackerOri⁻¹ * worldCorrection * trackerOri
    const Eigen::Quaternionf trackerOriInv = trackerOri.inverse();
    const Eigen::Quaternionf localCorrection = trackerOriInv * worldCorrection * trackerOri;

    // Apply to the local axis
    m_opticalAxisLocal = (localCorrection * m_opticalAxisLocal).normalized();
    m_opticalFineAdjustDeg = correctionDeg;
    m_opticalFineAdjustApplied = true;

    mne::Log::info("Optical fine adjust applied: {}°\n    axis (local):   ({}, {}, {})", correctionDeg, m_opticalAxisLocal.x(), m_opticalAxisLocal.y(), m_opticalAxisLocal.z());

    emit opticalCalibrationChanged();
    return true;
}

//=============================================================================================================

void PolhemusCoregistration::clearOpticalFineAdjust()
{
    if (!m_opticalFineAdjustApplied)
        return;

    m_opticalAxisLocal = m_opticalAxisPreFineAdjust;
    m_opticalFineAdjustApplied = false;
    m_opticalFineAdjustDeg = 0.0f;

    mne::Log::info("Optical fine adjust cleared — axis restored to original calibration.");
    emit opticalCalibrationChanged();
}

//=============================================================================================================
// Session persistence helpers
//=============================================================================================================

namespace
{

void saveVec3(QSettings& s, const QString& key, const Eigen::Vector3f& v)
{
    s.setValue(key + "/x", static_cast<double>(v.x()));
    s.setValue(key + "/y", static_cast<double>(v.y()));
    s.setValue(key + "/z", static_cast<double>(v.z()));
}

Eigen::Vector3f loadVec3(const QSettings& s, const QString& key)
{
    return Eigen::Vector3f(
        static_cast<float>(s.value(key + "/x", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/y", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/z", 0.0).toDouble()));
}

// Column-major floats, the layout earlier sessions stored.
void saveMat4(QSettings& s, const QString& key, const Eigen::Matrix4f& m)
{
    QByteArray data(reinterpret_cast<const char*>(m.data()), 16 * sizeof(float));
    s.setValue(key, data);
}

Eigen::Matrix4f loadMat4(const QSettings& s, const QString& key)
{
    QByteArray data = s.value(key).toByteArray();
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    if (data.size() == 16 * static_cast<int>(sizeof(float)))
        std::memcpy(m.data(), data.constData(), 16 * sizeof(float));
    return m;
}

void saveQuat(QSettings& s, const QString& key, const Eigen::Quaternionf& q)
{
    s.setValue(key + "/w", static_cast<double>(q.w()));
    s.setValue(key + "/x", static_cast<double>(q.x()));
    s.setValue(key + "/y", static_cast<double>(q.y()));
    s.setValue(key + "/z", static_cast<double>(q.z()));
}

Eigen::Quaternionf loadQuat(const QSettings& s, const QString& key)
{
    return Eigen::Quaternionf(
        static_cast<float>(s.value(key + "/w", 1.0).toDouble()),
        static_cast<float>(s.value(key + "/x", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/y", 0.0).toDouble()),
        static_cast<float>(s.value(key + "/z", 0.0).toDouble()));
}

} // anonymous namespace

//=============================================================================================================

void PolhemusCoregistration::saveSessionState(QSettings& settings, const QString& prefix) const
{
    settings.beginGroup(prefix);

    // Stations & axis mirror
    settings.setValue("trackerStation", m_trackerStation);
    settings.setValue("penStation", m_penStation);
    settings.setValue("probeStation", m_probeStation);
    settings.setValue("mirrorX", m_mirrorX);
    settings.setValue("mirrorY", m_mirrorY);

    // Pen fiducials
    // Indexed by FiducialId (LPA = 1, NAS = 2, RPA = 3)
    const char* fidNames[] = {"", "LPA", "NAS", "RPA"};
    for (int i = 1; i < 4; ++i) {
        settings.setValue(QString("hasPenFid/%1").arg(fidNames[i]), m_hasPenFid[i]);
        if (m_hasPenFid[i])
            saveVec3(settings, QString("penFid/%1").arg(fidNames[i]), m_penFid[i]);
    }

    // Model fiducials
    for (int i = 1; i < 4; ++i) {
        settings.setValue(QString("hasModelFid/%1").arg(fidNames[i]), m_hasModelFid[i]);
        if (m_hasModelFid[i])
            saveVec3(settings, QString("modelFid/%1").arg(fidNames[i]), m_modelFid[i]);
    }

    // Vertex
    settings.setValue("hasPenVertex", m_hasPenVertex);
    if (m_hasPenVertex)
        saveVec3(settings, "penVertex", m_penVertex);
    settings.setValue("hasModelVertex", m_hasModelVertex);
    if (m_hasModelVertex)
        saveVec3(settings, "modelVertex", m_modelVertex);

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

        // Fine adjustment
        settings.setValue("opticalFineAdjustApplied", m_opticalFineAdjustApplied);
        if (m_opticalFineAdjustApplied) {
            saveVec3(settings, "opticalAxisPreFineAdjust", m_opticalAxisPreFineAdjust);
            settings.setValue("opticalFineAdjustDeg", static_cast<double>(m_opticalFineAdjustDeg));
        }
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

bool PolhemusCoregistration::restoreSessionState(QSettings& settings, const QString& prefix)
{
    settings.beginGroup(prefix);
    if (settings.allKeys().isEmpty()) {
        settings.endGroup();
        return false;
    }

    // Stations & axis mirror
    m_trackerStation = settings.value("trackerStation", m_trackerStation).toInt();
    m_penStation = settings.value("penStation", m_penStation).toInt();
    m_probeStation = settings.value("probeStation", m_probeStation).toInt();
    m_mirrorX = settings.value("mirrorX", false).toBool();
    m_mirrorY = settings.value("mirrorY", false).toBool();

    // Pen fiducials
    // Indexed by FiducialId (LPA = 1, NAS = 2, RPA = 3)
    const char* fidNames[] = {"", "LPA", "NAS", "RPA"};
    for (int i = 1; i < 4; ++i) {
        m_hasPenFid[i] = settings.value(QString("hasPenFid/%1").arg(fidNames[i]), false).toBool();
        if (m_hasPenFid[i])
            m_penFid[i] = loadVec3(settings, QString("penFid/%1").arg(fidNames[i]));
    }

    // Model fiducials
    for (int i = 1; i < 4; ++i) {
        m_hasModelFid[i] = settings.value(QString("hasModelFid/%1").arg(fidNames[i]), false).toBool();
        if (m_hasModelFid[i])
            m_modelFid[i] = loadVec3(settings, QString("modelFid/%1").arg(fidNames[i]));
    }

    // Vertex
    m_hasPenVertex = settings.value("hasPenVertex", false).toBool();
    if (m_hasPenVertex)
        m_penVertex = loadVec3(settings, "penVertex");
    m_hasModelVertex = settings.value("hasModelVertex", false).toBool();
    if (m_hasModelVertex)
        m_modelVertex = loadVec3(settings, "modelVertex");

    // Calibration offset
    m_offsetTranslation = loadVec3(settings, "offsetTranslation");
    m_offsetRotation = loadQuat(settings, "offsetRotation");

    // Pen tip offset
    m_penTipOffset = loadVec3(settings, "penTipOffset");
    m_tipOffsetEnabled = settings.value("tipOffsetEnabled", false).toBool();

    // Optical calibration
    m_knownTrackerToObjectiveDist = static_cast<float>(settings.value("knownTrackerToObjectiveDist", 0.200).toDouble());
    m_hasObjectiveCenter = settings.value("hasObjectiveCenter", false).toBool();
    if (m_hasObjectiveCenter)
        m_objectiveCenterLocal = loadVec3(settings, "objectiveCenterLocal");
    m_opticalCalibValid = settings.value("opticalCalibValid", false).toBool();
    if (m_opticalCalibValid) {
        m_opticalAxisLocal = loadVec3(settings, "opticalAxisLocal");
        m_opticalCenterLocal = loadVec3(settings, "opticalCenterLocal");
        m_opticalCalibResidualMm = static_cast<float>(settings.value("opticalCalibResidualMm", 0.0).toDouble());
        m_opticalCalibDepthSpreadMm = static_cast<float>(settings.value("opticalCalibDepthSpreadMm", 0.0).toDouble());

        // Fine adjustment
        m_opticalFineAdjustApplied = settings.value("opticalFineAdjustApplied", false).toBool();
        if (m_opticalFineAdjustApplied) {
            m_opticalAxisPreFineAdjust = loadVec3(settings, "opticalAxisPreFineAdjust");
            m_opticalFineAdjustDeg = static_cast<float>(settings.value("opticalFineAdjustDeg", 0.0).toDouble());
        }
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
        m_headToWorld = loadMat4(settings, "headToWorld");
        m_worldToModel = loadMat4(settings, "worldToModel");
    }

    settings.endGroup();

    // Re-populate the AcquiredPoints list so that countOf() / UI status
    // reflect the restored fiducials.
    if (m_pPoints) {
        const char* fidLabels[] = {"", "LPA", "NAS", "RPA"};
        for (int i = 1; i <= 3; ++i) {
            m_pPoints->removeFiducial(static_cast<FiducialId>(i));
            if (m_hasPenFid[i]) {
                DigitizedPoint dp;
                dp.kind = PointKind::Fiducial;
                dp.label = QString::fromLatin1(fidLabels[i]);
                dp.identNumber = i;
                dp.position = m_penFid[i];
                m_pPoints->append(dp);
            }
        }
    }

    if (m_registrationValid)
        emit registrationChanged();

    return m_registrationValid;
}
