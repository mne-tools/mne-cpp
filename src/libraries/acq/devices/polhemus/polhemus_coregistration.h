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
 *           @ref compute_registration to obtain the head→device rigid
 *           transform (@ref head_to_device). This matrix can be used
 *           directly by a neuro-navigation pipeline.
 *
 *           The head coordinate frame follows the standard MEG/EEG convention:
 *             - Origin at the midpoint of LPA and RPA.
 *             - +X toward the nasion.
 *             - +Y toward LPA (patient's left).
 *             - +Z superior (up).
 *
 *           The tracker-to-device calibration offset
 *           (@ref set_tracker_to_device_offset) describes the rigid transform
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

#include <base/json.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <utility>
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
 *   - The computed head–device transform after @ref compute_registration.
 *
 * Events (new poses, button presses, calibration progress) reach the caller through the callbacks of a
 * @ref Listener; a Qt application forwards them as signals.
 *
 * @snippet ex_acq_polhemus/main.cpp polhemus_coregistration_usage
 */
class ACQPOLHEMUSSHARED_EXPORT PolhemusCoregistration
{
public:
    enum class PivotState
    {
        Idle,
        WaitingForStart,
        Collecting,
        Done
    };

    /**
     * @brief The events of a coregistration session; empty callbacks are skipped.
     */
    struct Listener
    {
        std::function<void(const Eigen::Matrix4f& device_to_world)> device_pose_changed;                                /**< New tracker pose. */
        std::function<void(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)> pen_pose_changed;   /**< New pen pose. */
        std::function<void(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)> probe_pose_changed; /**< New probe pose. */
        std::function<void(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)> pen_button_pressed; /**< Pen button pressed. */
        std::function<void()> registration_changed;                                                                     /**< Registration computed, reset or restored. */
        std::function<void(PivotState state)> pivot_state_changed;                                                      /**< Pivot calibration state. */
        std::function<void(const Eigen::Vector3f& offset, float residual_mm)> pivot_calibration_done;                   /**< Pivot calibration solved. */
        std::function<void(int sample_count, float angular_span_deg)> pivot_sample_collected;                           /**< Pivot sample taken. */
        std::function<void()> optical_calibration_changed;                                                              /**< Optical calibration changed. */
    };

    /**
     * Sets the callbacks for the events of this session.
     *
     * @param[in] listener   The callbacks; empty ones are skipped.
     */
    void set_listener(Listener listener)
    {
        m_listener = std::move(listener);
    }

    //=========================================================================================================
    // Station assignment
    //=========================================================================================================

    void set_tracker_station(int station);
    void set_pen_station(int station);
    void set_probe_station(int station);

    int tracker_station() const
    {
        return m_trackerStation;
    }
    int pen_station() const
    {
        return m_penStation;
    }
    int probe_station() const
    {
        return m_probeStation;
    }

    //=========================================================================================================
    // Pen tip offset (sensor body frame → tip)
    //=========================================================================================================

    void set_pen_tip_offset(const Eigen::Vector3f& offset)
    {
        m_penTipOffset = offset;
    }
    Eigen::Vector3f pen_tip_offset() const
    {
        return m_penTipOffset;
    }
    void set_tip_offset_enabled(bool on)
    {
        m_tipOffsetEnabled = on;
    }
    bool tip_offset_enabled() const
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
     * @param[in] mirror_x True to negate the X coordinate of incoming positions.
     * @param[in] mirror_y True to negate the Y coordinate of incoming positions.
     */
    void set_axis_mirror(bool mirror_x, bool mirror_y)
    {
        m_mirrorX = mirror_x;
        m_mirrorY = mirror_y;
    }
    bool mirror_x() const
    {
        return m_mirrorX;
    }
    bool mirror_y() const
    {
        return m_mirrorY;
    }

    //=========================================================================================================
    // Pivot calibration — determine pen tip offset by pivoting pen around its tip
    //=========================================================================================================

    void start_pivot_calibration();
    void cancel_pivot_calibration();
    PivotState pivot_state() const
    {
        return m_pivotState;
    }
    int pivot_sample_count() const
    {
        return static_cast<int>(m_pivotPositions.size());
    }
    float pivot_residual_mm() const
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
    void set_tracker_to_device_offset(const Eigen::Vector3f& translation,
                                      const Eigen::Quaternionf& rotation);

    Eigen::Vector3f tracker_to_device_translation() const
    {
        return m_offsetTranslation;
    }
    Eigen::Quaternionf tracker_to_device_rotation() const
    {
        return m_offsetRotation;
    }

    //=========================================================================================================
    // Optical path calibration — determine the microscope optical axis
    // relative to the tracker sensor.
    //
    // Workflow:
    //   1. Move the OPMI to a position, aim at a point on the head surface,
    //      mark the focus point with the stylus pen → capture_optical_calib_sample().
    //   2. Repeat at 2–3 different positions / focal distances.
    //   3. Call solve_optical_calibration() to fit a 3D line through the focus
    //      points in the tracker's local frame, yielding the optical axis
    //      direction and the optical center offset from the tracker.
    //=========================================================================================================

    struct OpticalCalibSample
    {
        Eigen::Vector3f tracker_pos = Eigen::Vector3f::Zero();           ///< Tracker position in world frame (metres).
        Eigen::Quaternionf tracker_ori = Eigen::Quaternionf::Identity(); ///< Tracker orientation in world frame.
        Eigen::Vector3f focus_point = Eigen::Vector3f::Zero();           ///< Stylus focus point in world frame (metres).
    };

    /**
     * @brief Record one calibration sample (current tracker pose + current pen position).
     * @return @c true if both pen and tracker data are available, @c false otherwise.
     */
    bool capture_optical_calib_sample();

    /** @return Number of calibration samples recorded so far. */
    int optical_calib_sample_count() const
    {
        return static_cast<int>(m_opticalCalibSamples.size());
    }

    /** Discard all calibration samples and reset the optical calibration. */
    void clear_optical_calib_samples();

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
    bool capture_objective_center();

    /** @return Whether an objective center has been captured. */
    bool has_objective_center() const
    {
        return m_hasObjectiveCenter;
    }

    /** Clear the captured objective center. */
    void clear_objective_center();

    /**
     * Captured objective center in tracker body frame (metres).
     *
     * @return Objective center in the tracker body frame in metres; only meaningful if has_objective_center() is true.
     */
    Eigen::Vector3f objective_center_local() const
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
    bool solve_optical_calibration();

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
    void set_known_tracker_to_objective_distance(float metres)
    {
        m_knownTrackerToObjectiveDist = metres;
    }
    float known_tracker_to_objective_distance() const
    {
        return m_knownTrackerToObjectiveDist;
    }

    /** @return Whether a valid optical calibration has been computed. */
    bool optical_calibration_valid() const
    {
        return m_opticalCalibValid;
    }

    /**
     * Direction of the optical axis in the tracker body frame (unit vector).
     *
     * @return Unit optical-axis direction in the tracker body frame; only meaningful if optical_calibration_valid() is true.
     */
    Eigen::Vector3f optical_axis_local() const
    {
        return m_opticalAxisLocal;
    }

    /**
     * Position of the optical center in the tracker body frame (metres).
     *
     * @return Optical center in the tracker body frame in metres.
     */
    Eigen::Vector3f optical_center_local() const
    {
        return m_opticalCenterLocal;
    }

    /**
     * RMS residual of the last optical calibration (mm).
     *
     * @return RMS distance of the focus points from the fitted optical axis in millimetres.
     */
    float optical_calib_residual_mm() const
    {
        return m_opticalCalibResidualMm;
    }

    /**
     * Depth spread of calibration samples along the optical axis (mm).
     *
     * @return Extent of the calibration focus points along the optical axis in millimetres.
     */
    float optical_calib_depth_spread_mm() const
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
    bool apply_optical_axis_fine_adjust(const Eigen::Vector3f& targetWorldPos,
                                        float& correctionDeg);

    /** @return Whether a fine adjustment has been applied. */
    bool optical_fine_adjust_applied() const
    {
        return m_opticalFineAdjustApplied;
    }

    /** @return The angle of the last fine adjustment (degrees). */
    float optical_fine_adjust_deg() const
    {
        return m_opticalFineAdjustDeg;
    }

    /** Undo the last fine adjustment, restoring the original solved axis. */
    void clear_optical_fine_adjust();

    /**
     * @brief Compute the current optical axis ray in Polhemus world frame.
     *
     * @param[out] origin     Ray origin (optical center in world frame).
     * @param[out] direction  Ray direction (optical axis in world frame, unit vector).
     * @return @c true if tracker data and calibration are available.
     */
    bool optical_ray_in_world(Eigen::Vector3f& origin, Eigen::Vector3f& direction) const;

    /**
     * Returns the "up" direction of the optical path in world coordinates.
     * This is the tracker's local Y axis rotated into the world frame,
     * orthogonalised against the optical axis so it can be used directly
     * as an orientation hint for projected overlays.
     *
     * @param[out] up  Unit vector pointing "up" in the microscope view.
     * @return @c true if tracker data and calibration are available.
     */
    bool optical_up_in_world(Eigen::Vector3f& up) const;

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

    AcquiredPoints& acquired_points()
    {
        return m_points;
    }
    const AcquiredPoints& acquired_points() const
    {
        return m_points;
    }

    //=========================================================================================================
    // Explicit capture API
    //=========================================================================================================

    bool capture_current_pen_position_as_fiducial(FiducialId id);
    bool capture_current_pen_position_as_head_shape();

    //=========================================================================================================
    // Model (BEM / FIFF) fiducials — target landmarks for SVD coregistration
    //=========================================================================================================

    void set_model_fiducial(FiducialId id, const Eigen::Vector3f& posInModel);
    bool has_model_fiducial(FiducialId id) const;
    bool has_all_model_fiducials() const;
    Eigen::Vector3f model_fiducial(FiducialId id) const;

    bool has_all_pen_fiducials() const;

    //=========================================================================================================
    // Vertex (CZ / top of head) — 4th reference point for orientation validation
    //=========================================================================================================

    void set_model_vertex(const Eigen::Vector3f& pos)
    {
        m_modelVertex = pos;
        m_hasModelVertex = true;
    }
    bool has_model_vertex() const
    {
        return m_hasModelVertex;
    }
    Eigen::Vector3f model_vertex() const
    {
        return m_modelVertex;
    }

    bool capture_current_pen_position_as_vertex();
    bool has_pen_vertex() const
    {
        return m_hasPenVertex;
    }
    Eigen::Vector3f pen_vertex() const
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
    bool compute_registration();

    /**
     * @brief Reset the registration state (head_to_world, head_to_device) to identity.
     *        Call this when the user clears all fiducials.
     */
    void reset_registration();

    //=========================================================================================================
    // Session persistence — save/restore registration state across app restarts
    //=========================================================================================================

    /**
     * @return Stations, fiducials, calibrations and the registration, to store and give to restore_session_state().
     */
    JsonObject session_state() const;

    /**
     * Restores a state from session_state(); entries it lacks keep their current values.
     *
     * @param[in] state   The stored state.
     * @return Whether it holds a valid registration.
     */
    bool restore_session_state(const JsonObject& state);

    bool registration_valid() const
    {
        return m_registrationValid;
    }

    //=========================================================================================================
    // Current state accessors
    //=========================================================================================================

    Eigen::Matrix4f device_to_world() const
    {
        return m_deviceToWorld;
    }
    Eigen::Matrix4f head_to_world() const
    {
        return m_headToWorld;
    }
    Eigen::Matrix4f head_to_device() const
    {
        return m_headToDevice;
    }
    Eigen::Matrix4f world_to_model() const
    {
        return m_worldToModel;
    }
    Eigen::Vector3f pen_position() const
    {
        return m_penPosition;
    }
    Eigen::Quaternionf pen_orientation() const
    {
        return m_penOrientation;
    }
    bool have_live_pen_position() const
    {
        return m_havePenPos;
    }

    Eigen::Vector3f probe_position() const
    {
        return m_probePosition;
    }
    Eigen::Quaternionf probe_orientation() const
    {
        return m_probeOrientation;
    }
    bool have_live_probe_position() const
    {
        return m_haveProbePos;
    }

private:
    Eigen::Matrix4f buildDevicePose(const Eigen::Vector3f& tracker_pos,
                                    const Eigen::Quaternionf& tracker_ori) const;
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

    AcquiredPoints m_points;
    Listener m_listener;

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
