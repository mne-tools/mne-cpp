//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_coregistration.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
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

#ifndef ACQ_POLHEMUS_COREGISTRATION_H
#define ACQ_POLHEMUS_COREGISTRATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_polhemus_global.h"
#include "acquired_points.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSettings>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Head–device coregistration engine driven by a Polhemus Fastrak.
 *
 * It takes the station poses and stylus button presses of a polhemus:// stream (on_pose(), on_button()) and maintains:
 *   - A continuously updated device-to-world transform (from the tracker
 *     station + user-supplied calibration offset).
 *   - An @ref AcquiredPoints store filled by explicit capture calls or
 *     automatically on pen-button press.
 *   - The computed head–device transform after @ref computeRegistration.
 *
 * @snippet ex_acq_polhemus/main.cpp polhemus_coregistration_usage
 */
class ACQPOLHEMUSSHARED_EXPORT PolhemusCoregistration : public QObject
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
    void setProbeStation(int station);

    int trackerStation() const
    {
        return m_trackerStation;
    }
    int penStation() const
    {
        return m_penStation;
    }
    int probeStation() const
    {
        return m_probeStation;
    }

    //=========================================================================================================
    // Pen tip offset (sensor body frame → tip)
    //=========================================================================================================

    void setPenTipOffset(const Eigen::Vector3f& offset)
    {
        m_penTipOffset = offset;
    }
    Eigen::Vector3f penTipOffset() const
    {
        return m_penTipOffset;
    }
    void setTipOffsetEnabled(bool on)
    {
        m_tipOffsetEnabled = on;
    }
    bool tipOffsetEnabled() const
    {
        return m_tipOffsetEnabled;
    }

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
     *
     * @param[in] mirrorX True to negate the X coordinate of incoming positions.
     * @param[in] mirrorY True to negate the Y coordinate of incoming positions.
     */
    void setAxisMirror(bool mirrorX, bool mirrorY)
    {
        m_mirrorX = mirrorX;
        m_mirrorY = mirrorY;
    }
    bool mirrorX() const
    {
        return m_mirrorX;
    }
    bool mirrorY() const
    {
        return m_mirrorY;
    }

    //=========================================================================================================
    // Pivot calibration — determine pen tip offset by pivoting pen around its tip
    //=========================================================================================================

    enum class PivotState
    {
        Idle,
        WaitingForStart,
        Collecting,
        Done
    };

    void startPivotCalibration();
    void cancelPivotCalibration();
    PivotState pivotState() const
    {
        return m_pivotState;
    }
    int pivotSampleCount() const
    {
        return static_cast<int>(m_pivotPositions.size());
    }
    float pivotResidualMm() const
    {
        return m_pivotResidualMm;
    }

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
    void setTrackerToDeviceOffset(const Eigen::Vector3f& translation,
                                  const Eigen::Quaternionf& rotation);

    Eigen::Vector3f trackerToDeviceTranslation() const
    {
        return m_offsetTranslation;
    }
    Eigen::Quaternionf trackerToDeviceRotation() const
    {
        return m_offsetRotation;
    }

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

    struct OpticalCalibSample
    {
        Eigen::Vector3f trackerPos = Eigen::Vector3f::Zero();           ///< Tracker position in world frame (metres).
        Eigen::Quaternionf trackerOri = Eigen::Quaternionf::Identity(); ///< Tracker orientation in world frame.
        Eigen::Vector3f focusPoint = Eigen::Vector3f::Zero();           ///< Stylus focus point in world frame (metres).
    };

    /**
     * @brief Record one calibration sample (current tracker pose + current pen position).
     * @return @c true if both pen and tracker data are available, @c false otherwise.
     */
    bool captureOpticalCalibSample();

    /** @return Number of calibration samples recorded so far. */
    int opticalCalibSampleCount() const
    {
        return static_cast<int>(m_opticalCalibSamples.size());
    }

    /** Discard all calibration samples and reset the optical calibration. */
    void clearOpticalCalibSamples();

    /**
     * @brief Capture the current pen position as the objective lens center.
     *
     * Touch the pen stylus to the center of the OPMI objective lens while
     * the tracker is live.  The point is stored in the tracker's local frame
     * and used by the solver as a direct measurement of the optical center,
     * replacing the sphere-distance constraint.
     *
     * @return @c true on success, @c false if pen or tracker data unavailable.
     */
    bool captureObjectiveCenter();

    /** @return Whether an objective center has been captured. */
    bool hasObjectiveCenter() const
    {
        return m_hasObjectiveCenter;
    }

    /** Clear the captured objective center. */
    void clearObjectiveCenter();

    /**
     * Captured objective center in tracker body frame (metres).
     *
     * @return Objective center in the tracker body frame in metres; only meaningful if hasObjectiveCenter() is true.
     */
    Eigen::Vector3f objectiveCenterLocal() const
    {
        return m_objectiveCenterLocal;
    }

    /**
     * @brief Fit a 3D line through the focus points in tracker-local frame.
     *
     * Requires at least 2 samples. Computes the optical axis direction
     * and the optical center offset in the tracker body frame.
     *
     * @return @c true on success; @c false if insufficient or degenerate data.
     */
    bool solveOpticalCalibration();

    /**
     * @brief Set the known perpendicular distance from tracker sensor to
     *        the objective center (metres).
     *
     * When nonzero, the solver constrains the optical center to lie at this
     * distance from the tracker origin, vastly improving accuracy.
     * Default: 0.200 m (~200 mm for ZEISS Kinevo).  Set to 0 to disable.
     *
     * @param[in] metres Tracker-to-objective distance in metres; 0 disables the constraint.
     */
    void setKnownTrackerToObjectiveDistance(float metres)
    {
        m_knownTrackerToObjectiveDist = metres;
    }
    float knownTrackerToObjectiveDistance() const
    {
        return m_knownTrackerToObjectiveDist;
    }

    /** @return Whether a valid optical calibration has been computed. */
    bool opticalCalibrationValid() const
    {
        return m_opticalCalibValid;
    }

    /**
     * Direction of the optical axis in the tracker body frame (unit vector).
     *
     * @return Unit optical-axis direction in the tracker body frame; only meaningful if opticalCalibrationValid() is true.
     */
    Eigen::Vector3f opticalAxisLocal() const
    {
        return m_opticalAxisLocal;
    }

    /**
     * Position of the optical center in the tracker body frame (metres).
     *
     * @return Optical center in the tracker body frame in metres.
     */
    Eigen::Vector3f opticalCenterLocal() const
    {
        return m_opticalCenterLocal;
    }

    /**
     * RMS residual of the last optical calibration (mm).
     *
     * @return RMS distance of the focus points from the fitted optical axis in millimetres.
     */
    float opticalCalibResidualMm() const
    {
        return m_opticalCalibResidualMm;
    }

    /**
     * Depth spread of calibration samples along the optical axis (mm).
     *
     * @return Extent of the calibration focus points along the optical axis in millimetres.
     */
    float opticalCalibDepthSpreadMm() const
    {
        return m_opticalCalibDepthSpreadMm;
    }

    /**
     * @brief Fine-adjust the optical axis so it passes through a known
     *        world-frame point (e.g. the probe tip touched with the stylus).
     *
     * The correction rotates @c m_opticalAxisLocal in the tracker body
     * frame so that a ray from the optical center would pass through
     * @a targetWorldPos at the current tracker pose.
     *
     * @param targetWorldPos  Desired focus point in Polhemus world frame.
     * @param[out] correctionDeg  Angle of the applied correction (degrees).
     * @return @c true on success; @c false if calibration or tracker data
     *         is unavailable, or the correction would exceed 10°.
     */
    bool applyOpticalAxisFineAdjust(const Eigen::Vector3f& targetWorldPos,
                                    float& correctionDeg);

    /** @return Whether a fine adjustment has been applied. */
    bool opticalFineAdjustApplied() const
    {
        return m_opticalFineAdjustApplied;
    }

    /** @return The angle of the last fine adjustment (degrees). */
    float opticalFineAdjustDeg() const
    {
        return m_opticalFineAdjustDeg;
    }

    /** Undo the last fine adjustment, restoring the original solved axis. */
    void clearOpticalFineAdjust();

    /**
     * @brief Compute the current optical axis ray in Polhemus world frame.
     *
     * @param[out] origin     Ray origin (optical center in world frame).
     * @param[out] direction  Ray direction (optical axis in world frame, unit vector).
     * @return @c true if tracker data and calibration are available.
     */
    bool opticalRayInWorld(Eigen::Vector3f& origin, Eigen::Vector3f& direction) const;

    /**
     * Returns the "up" direction of the optical path in world coordinates.
     * This is the tracker's local Y axis rotated into the world frame,
     * orthogonalised against the optical axis so it can be used directly
     * as an orientation hint for projected overlays.
     *
     * @param[out] up  Unit vector pointing "up" in the microscope view.
     * @return @c true if tracker data and calibration are available.
     */
    bool opticalUpInWorld(Eigen::Vector3f& up) const;

    //=========================================================================================================
    // Digitizer input
    //=========================================================================================================

    /**
     * Takes the pose of a station, as a polhemus:// stream delivers it.
     *
     * @param[in] station       Station 1..4.
     * @param[in] position      Position in metres in the transmitter frame.
     * @param[in] orientation   Orientation of the sensor.
     */
    void on_pose(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);

    /**
     * Takes a stylus button press (event code 1 of a polhemus:// stream) at the pose of the station.
     *
     * @param[in] station       Station whose button was pressed; only the pen station counts.
     * @param[in] position      Position in metres in the transmitter frame.
     * @param[in] orientation   Orientation of the sensor.
     */
    void on_button(int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);

    //=========================================================================================================
    // Digitized-point store
    //=========================================================================================================

    AcquiredPoints* acquiredPoints() const
    {
        return m_pPoints;
    }

    //=========================================================================================================
    // Explicit capture API
    //=========================================================================================================

    bool captureCurrentPenPositionAsFiducial(FiducialId id);
    bool captureCurrentPenPositionAsHeadShape();

    //=========================================================================================================
    // Model (BEM / FIFF) fiducials — target landmarks for SVD coregistration
    //=========================================================================================================

    void setModelFiducial(FiducialId id, const Eigen::Vector3f& posInModel);
    bool hasModelFiducial(FiducialId id) const;
    bool hasAllModelFiducials() const;
    Eigen::Vector3f modelFiducial(FiducialId id) const;

    bool hasAllPenFiducials() const;

    //=========================================================================================================
    // Vertex (CZ / top of head) — 4th reference point for orientation validation
    //=========================================================================================================

    void setModelVertex(const Eigen::Vector3f& pos)
    {
        m_modelVertex = pos;
        m_hasModelVertex = true;
    }
    bool hasModelVertex() const
    {
        return m_hasModelVertex;
    }
    Eigen::Vector3f modelVertex() const
    {
        return m_modelVertex;
    }

    bool captureCurrentPenPositionAsVertex();
    bool hasPenVertex() const
    {
        return m_hasPenVertex;
    }
    Eigen::Vector3f penVertex() const
    {
        return m_penVertex;
    }

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

    //=========================================================================================================
    // Session persistence — save/restore registration state across app restarts
    //=========================================================================================================

    void saveSessionState(QSettings& settings, const QString& prefix = QStringLiteral("polhemus")) const;
    bool restoreSessionState(QSettings& settings, const QString& prefix = QStringLiteral("polhemus"));

    bool registrationValid() const
    {
        return m_registrationValid;
    }

    //=========================================================================================================
    // Current state accessors
    //=========================================================================================================

    Eigen::Matrix4f deviceToWorld() const
    {
        return m_deviceToWorld;
    }
    Eigen::Matrix4f headToWorld() const
    {
        return m_headToWorld;
    }
    Eigen::Matrix4f headToDevice() const
    {
        return m_headToDevice;
    }
    Eigen::Matrix4f worldToModel() const
    {
        return m_worldToModel;
    }
    Eigen::Vector3f penPosition() const
    {
        return m_penPosition;
    }
    Eigen::Quaternionf penOrientation() const
    {
        return m_penOrientation;
    }
    bool haveLivePenPosition() const
    {
        return m_havePenPos;
    }

    Eigen::Vector3f probePosition() const
    {
        return m_probePosition;
    }
    Eigen::Quaternionf probeOrientation() const
    {
        return m_probeOrientation;
    }
    bool haveLiveProbePosition() const
    {
        return m_haveProbePos;
    }

signals:
    void devicePoseChanged(const Eigen::Matrix4f& deviceToWorld);
    void penPoseChanged(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);
    void probePoseChanged(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);
    void penButtonPressed(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation);
    void registrationChanged();
    void pivotStateChanged(PolhemusCoregistration::PivotState state);
    void pivotCalibrationDone(const Eigen::Vector3f& offset, float residualMm);
    void pivotSampleCollected(int sampleCount, float angularSpanDeg);
    void opticalCalibrationChanged();

private:
    Eigen::Matrix4f buildDevicePose(const Eigen::Vector3f& trackerPos,
                                    const Eigen::Quaternionf& trackerOri) const;
    Eigen::Matrix4f buildHeadFrame() const;

    int m_trackerStation = 2;
    int m_penStation = 1;
    int m_probeStation = 3;

    Eigen::Vector3f m_offsetTranslation = Eigen::Vector3f::Zero();
    Eigen::Quaternionf m_offsetRotation = Eigen::Quaternionf::Identity();
    Eigen::Vector3f m_penTipOffset = Eigen::Vector3f::Zero(); // tip offset in sensor body frame (metres)
    bool m_tipOffsetEnabled = false;
    bool m_mirrorX = false; // negate X of incoming Polhemus positions
    bool m_mirrorY = false; // negate Y of incoming Polhemus positions

    AcquiredPoints* m_pPoints = nullptr;

    Eigen::Vector3f m_penPosition = Eigen::Vector3f::Zero();
    Eigen::Quaternionf m_penOrientation = Eigen::Quaternionf::Identity();
    bool m_havePenPos = false;

    Eigen::Vector3f m_probePosition = Eigen::Vector3f::Zero();
    Eigen::Quaternionf m_probeOrientation = Eigen::Quaternionf::Identity();
    bool m_haveProbePos = false;

    Eigen::Matrix4f m_deviceToWorld = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f m_headToWorld = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f m_headToDevice = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f m_worldToModel = Eigen::Matrix4f::Identity();
    bool m_registrationValid = false;

    // Pen fiducials (captured from stylus in Polhemus world frame)
    Eigen::Vector3f m_penFid[4] = {Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero()}; // indexed by FiducialId (1..3)
    bool m_hasPenFid[4] = {};

    // Model fiducials (from FIFF / BEM, in MRI/surface-RAS frame)
    Eigen::Vector3f m_modelFid[4] = {Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero()}; // indexed by FiducialId (1..3)
    bool m_hasModelFid[4] = {};

    // Vertex / CZ (top of head) for orientation validation
    Eigen::Vector3f m_modelVertex = Eigen::Vector3f::Zero();
    bool m_hasModelVertex = false;
    Eigen::Vector3f m_penVertex = Eigen::Vector3f::Zero();
    bool m_hasPenVertex = false;

    // Pivot calibration state
    PivotState m_pivotState = PivotState::Idle;
    std::vector<Eigen::Vector3f> m_pivotPositions;
    std::vector<Eigen::Quaternionf> m_pivotOrientations;
    float m_pivotResidualMm = 0.0f;

    // Optical path calibration state
    std::vector<OpticalCalibSample> m_opticalCalibSamples;
    Eigen::Vector3f m_opticalAxisLocal = Eigen::Vector3f::Zero();   // optical axis direction in tracker body frame
    Eigen::Vector3f m_opticalCenterLocal = Eigen::Vector3f::Zero(); // optical center position in tracker body frame
    float m_opticalCalibResidualMm = 0.0f;
    float m_opticalCalibDepthSpreadMm = 0.0f;
    bool m_opticalCalibValid = false;

    // Directly captured objective center (in tracker-local frame, metres).
    // When available, the solver uses this instead of the distance constraint.
    Eigen::Vector3f m_objectiveCenterLocal = Eigen::Vector3f::Zero(); // objective center in tracker body frame
    bool m_hasObjectiveCenter = false;

    // Fine adjustment — small correction rotation applied on top of the
    // solved optical axis to compensate for residual misalignment between
    // the tracked probe position and its appearance in the video.
    Eigen::Vector3f m_opticalAxisPreFineAdjust = Eigen::Vector3f::Zero(); // axis before fine-adjust (for undo)
    float m_opticalFineAdjustDeg = 0.0f;
    bool m_opticalFineAdjustApplied = false;

    // Known tracker-to-objective distance (metres) — fallback constraint when
    // no direct objective center capture is available.
    float m_knownTrackerToObjectiveDist = 0.200f; // metres

    bool solvePivotCalibration();
};

} // namespace mne

#endif // ACQ_POLHEMUS_COREGISTRATION_H
