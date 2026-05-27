//=============================================================================================================
/**
 * @file     polhemus_coregistration.h
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
 * @brief    Head–device coregistration using the Polhemus Fastrak.
 *
 *           Two Polhemus stations work together:
 *
 *           - **Tracker station** — a rigid sensor fixed to an external
 *             device (e.g. microscope) at a known calibration offset.
 *             Its continuous stream provides the live device pose in
 *             the Polhemus world (transmitter) frame.
 *
 *           - **Pen station** — the digitization stylus. The operator
 *             touches anatomical landmarks (NAS, LPA, RPA) and free
 *             head-surface points to build up an @ref AcquiredPoints store.
 *
 *           After capturing all three fiducials the caller invokes
 *           @ref computeRegistration to obtain the head→device rigid
 *           transform (@ref headToDevice). This matrix can be used
 *           directly by a neuro-navigation pipeline.
 *
 *           The head coordinate frame follows the standard MEG/EEG convention:
 *             - Origin at the midpoint of LPA and RPA.
 *             - +X toward the nasion.
 *             - +Y toward LPA (patient's left).
 *             - +Z superior (up).
 *
 *           The tracker-to-device calibration offset
 *           (@ref setTrackerToDeviceOffset) describes the rigid transform
 *           from the tracker sensor body frame to the device frame.
 *           It is determined once by a pivot-calibration procedure and
 *           stored in the application settings.
 */

#ifndef UTILS_POLHEMUS_COREGISTRATION_H
#define UTILS_POLHEMUS_COREGISTRATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"
#include "acquired_points.h"
#include "polhemus_connection.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QObject>
#include <QQuaternion>
#include <QVector3D>

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Head–device coregistration engine driven by a Polhemus Fastrak.
 *
 * The class wires itself to a @ref PolhemusConnection and maintains:
 *   - A continuously updated device-to-world transform (from the tracker
 *     station + user-supplied calibration offset).
 *   - An @ref AcquiredPoints store filled by explicit capture calls or
 *     automatically on pen-button press.
 *   - The computed head–device transform after @ref computeRegistration.
 */
class UTILSSHARED_EXPORT PolhemusCoregistration : public QObject
{
    Q_OBJECT

public:
    explicit PolhemusCoregistration(QObject* parent = nullptr);
    ~PolhemusCoregistration() override = default;

    //=========================================================================================================
    // Station assignment
    //=========================================================================================================

    void setTrackerStation(int station);
    void setPenStation(int station);

    int trackerStation() const { return m_trackerStation; }
    int penStation()     const { return m_penStation; }

    //=========================================================================================================
    // Pen tip offset (sensor body frame → tip)
    //=========================================================================================================

    void setPenTipOffset(const QVector3D& offset) { m_penTipOffset = offset; }
    QVector3D penTipOffset() const { return m_penTipOffset; }
    void setTipOffsetEnabled(bool on) { m_tipOffsetEnabled = on; }
    bool tipOffsetEnabled() const { return m_tipOffsetEnabled; }

    //=========================================================================================================
    // Axis mirroring — compensate for transmitter placement
    //=========================================================================================================

    /**
     * @brief Negate X and/or Y of incoming Polhemus positions.
     *
     * The Polhemus Fastrak coordinate system depends on the physical
     * placement and orientation of the transmitter.  When the transmitter
     * is oriented such that its axes are mirrored relative to the
     * expected neuroscience convention, enable the corresponding mirror
     * flags here.
     */
    void setAxisMirror(bool mirrorX, bool mirrorY) { m_mirrorX = mirrorX; m_mirrorY = mirrorY; }
    bool mirrorX() const { return m_mirrorX; }
    bool mirrorY() const { return m_mirrorY; }

    //=========================================================================================================
    // Pivot calibration — determine pen tip offset by pivoting pen around its tip
    //=========================================================================================================

    enum class PivotState { Idle, WaitingForStart, Collecting, Done };

    void startPivotCalibration();
    void cancelPivotCalibration();
    PivotState pivotState() const { return m_pivotState; }
    int pivotSampleCount() const { return static_cast<int>(m_pivotPositions.size()); }
    float pivotResidualMm() const { return m_pivotResidualMm; }

    //=========================================================================================================
    // Tracker-to-device calibration offset
    //=========================================================================================================

    /**
     * @brief Set the rigid offset from the tracker sensor body frame to the
     *        device frame.
     *
     * @param translation  Translation in metres, expressed in the tracker body frame.
     * @param rotation     Rotation from tracker body frame to device frame.
     */
    void setTrackerToDeviceOffset(const QVector3D& translation,
                                  const QQuaternion& rotation);

    QVector3D   trackerToDeviceTranslation() const { return m_offsetTranslation; }
    QQuaternion trackerToDeviceRotation()    const { return m_offsetRotation; }

    //=========================================================================================================
    // Optical path calibration — determine the microscope optical axis
    // relative to the tracker sensor.
    //
    // Workflow:
    //   1. Move the OPMI to a position, aim at a point on the head surface,
    //      mark the focus point with the stylus pen → captureOpticalCalibSample().
    //   2. Repeat at 2–3 different positions / focal distances.
    //   3. Call solveOpticalCalibration() to fit a 3D line through the focus
    //      points in the tracker's local frame, yielding the optical axis
    //      direction and the optical center offset from the tracker.
    //=========================================================================================================

    struct OpticalCalibSample {
        QVector3D   trackerPos;    ///< Tracker position in world frame (metres).
        QQuaternion trackerOri;    ///< Tracker orientation in world frame.
        QVector3D   focusPoint;    ///< Stylus focus point in world frame (metres).
    };

    /**
     * @brief Record one calibration sample (current tracker pose + current pen position).
     * @return @c true if both pen and tracker data are available, @c false otherwise.
     */
    bool captureOpticalCalibSample();

    /** @return Number of calibration samples recorded so far. */
    int opticalCalibSampleCount() const { return static_cast<int>(m_opticalCalibSamples.size()); }

    /** Discard all calibration samples and reset the optical calibration. */
    void clearOpticalCalibSamples();

    /**
     * @brief Fit a 3D line through the focus points in tracker-local frame.
     *
     * Requires at least 2 samples. Computes the optical axis direction
     * and the optical center offset in the tracker body frame.
     *
     * @return @c true on success; @c false if insufficient or degenerate data.
     */
    bool solveOpticalCalibration();

    /** @return Whether a valid optical calibration has been computed. */
    bool opticalCalibrationValid() const { return m_opticalCalibValid; }

    /** Direction of the optical axis in the tracker body frame (unit vector). */
    QVector3D opticalAxisLocal() const { return m_opticalAxisLocal; }

    /** Position of the optical center in the tracker body frame (metres). */
    QVector3D opticalCenterLocal() const { return m_opticalCenterLocal; }

    /** RMS residual of the last optical calibration (mm). */
    float opticalCalibResidualMm() const { return m_opticalCalibResidualMm; }

    /** Depth spread of calibration samples along the optical axis (mm). */
    float opticalCalibDepthSpreadMm() const { return m_opticalCalibDepthSpreadMm; }

    /**
     * @brief Compute the current optical axis ray in Polhemus world frame.
     *
     * @param[out] origin     Ray origin (optical center in world frame).
     * @param[out] direction  Ray direction (optical axis in world frame, unit vector).
     * @return @c true if tracker data and calibration are available.
     */
    bool opticalRayInWorld(QVector3D& origin, QVector3D& direction) const;

    //=========================================================================================================
    // Digitizer connection
    //=========================================================================================================

    void setConnection(PolhemusConnection* conn);
    PolhemusConnection* connection() const { return m_pConn; }

    //=========================================================================================================
    // Digitized-point store
    //=========================================================================================================

    AcquiredPoints* acquiredPoints() const { return m_pPoints; }

    //=========================================================================================================
    // Explicit capture API
    //=========================================================================================================

    bool captureCurrentPenPositionAsFiducial(FiducialId id);
    bool captureCurrentPenPositionAsHeadShape();

    //=========================================================================================================
    // Model (BEM / FIFF) fiducials — target landmarks for SVD coregistration
    //=========================================================================================================

    void setModelFiducial(FiducialId id, const QVector3D& posInModel);
    bool hasModelFiducial(FiducialId id) const;
    bool hasAllModelFiducials() const;
    QVector3D modelFiducial(FiducialId id) const;

    bool hasAllPenFiducials() const;

    //=========================================================================================================
    // Vertex (CZ / top of head) — 4th reference point for orientation validation
    //=========================================================================================================

    void setModelVertex(const QVector3D& pos) { m_modelVertex = pos; m_hasModelVertex = true; }
    bool hasModelVertex() const { return m_hasModelVertex; }
    QVector3D modelVertex() const { return m_modelVertex; }

    bool captureCurrentPenPositionAsVertex();
    bool hasPenVertex() const { return m_hasPenVertex; }
    QVector3D penVertex() const { return m_penVertex; }

    //=========================================================================================================
    // Registration
    //=========================================================================================================

    /**
     * @brief Compute the head→device rigid transform from the three
     *        captured fiducials (NAS, LPA, RPA).
     *
     * @return @c true on success; @c false if any fiducial is missing or the
     *         fiducials are degenerate (collinear).
     */
    bool computeRegistration();

    /**
     * @brief Reset the registration state (headToWorld, headToDevice) to identity.
     *        Call this when the user clears all fiducials.
     */
    void resetRegistration();

    bool registrationValid() const { return m_registrationValid; }

    //=========================================================================================================
    // Current state accessors
    //=========================================================================================================

    QMatrix4x4  deviceToWorld()    const { return m_deviceToWorld; }
    QMatrix4x4  headToWorld()      const { return m_headToWorld; }
    QMatrix4x4  headToDevice()     const { return m_headToDevice; }
    QMatrix4x4  worldToModel()     const { return m_worldToModel; }
    QVector3D   penPosition()      const { return m_penPosition; }
    QQuaternion penOrientation()   const { return m_penOrientation; }
    bool        haveLivePenPosition() const { return m_havePenPos; }

signals:
    void devicePoseChanged(const QMatrix4x4& deviceToWorld);
    void penPoseChanged(const QVector3D& position, const QQuaternion& orientation);
    void penButtonPressed(const QVector3D& position, const QQuaternion& orientation);
    void registrationChanged();
    void pivotStateChanged(PolhemusCoregistration::PivotState state);
    void pivotCalibrationDone(const QVector3D& offset, float residualMm);
    void pivotSampleCollected(int sampleCount, float angularSpanDeg);
    void opticalCalibrationChanged();

private slots:
    void onPointReceived(int station, const QVector3D& position, const QQuaternion& orientation);
    void onPenButtonPressedFromConn(int station, const QVector3D& position, const QQuaternion& orientation);

private:
    QMatrix4x4 buildDevicePose(const QVector3D& trackerPos,
                               const QQuaternion& trackerOri) const;
    QMatrix4x4 buildHeadFrame() const;

    int m_trackerStation = 2;
    int m_penStation     = 1;

    QVector3D   m_offsetTranslation;
    QQuaternion m_offsetRotation;
    QVector3D   m_penTipOffset;        // tip offset in sensor body frame (metres)
    bool        m_tipOffsetEnabled = false;
    bool        m_mirrorX = false;     // negate X of incoming Polhemus positions
    bool        m_mirrorY = false;     // negate Y of incoming Polhemus positions

    PolhemusConnection* m_pConn   = nullptr;
    AcquiredPoints*     m_pPoints = nullptr;

    QVector3D   m_penPosition;
    QQuaternion m_penOrientation;
    bool        m_havePenPos = false;

    QMatrix4x4  m_deviceToWorld;
    QMatrix4x4  m_headToWorld;
    QMatrix4x4  m_headToDevice;
    QMatrix4x4  m_worldToModel;
    bool        m_registrationValid = false;

    // Pen fiducials (captured from stylus in Polhemus world frame)
    QVector3D   m_penFid[4];          // indexed by FiducialId (1..3)
    bool        m_hasPenFid[4] = {};

    // Model fiducials (from FIFF / BEM, in MRI/surface-RAS frame)
    QVector3D   m_modelFid[4];        // indexed by FiducialId (1..3)
    bool        m_hasModelFid[4] = {};

    // Vertex / CZ (top of head) for orientation validation
    QVector3D   m_modelVertex;
    bool        m_hasModelVertex = false;
    QVector3D   m_penVertex;
    bool        m_hasPenVertex = false;

    // Pivot calibration state
    PivotState  m_pivotState = PivotState::Idle;
    std::vector<QVector3D>   m_pivotPositions;
    std::vector<QQuaternion> m_pivotOrientations;
    float m_pivotResidualMm = 0.0f;

    // Optical path calibration state
    std::vector<OpticalCalibSample> m_opticalCalibSamples;
    QVector3D m_opticalAxisLocal;           // optical axis direction in tracker body frame
    QVector3D m_opticalCenterLocal;         // optical center position in tracker body frame
    float     m_opticalCalibResidualMm = 0.0f;
    float     m_opticalCalibDepthSpreadMm = 0.0f;
    bool      m_opticalCalibValid = false;

    bool solvePivotCalibration();
};

} // namespace UTILSLIB

#endif // UTILS_POLHEMUS_COREGISTRATION_H
