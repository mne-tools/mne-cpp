//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     camera.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    OrbitCamera, the view presets and the projection and view matrices of the 3-D brain view.
 */

#ifndef VIZ_CAMERA_H
#define VIZ_CAMERA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Fixed camera directions of a pane of the 3-D view; the int values are the persisted preset numbers.
 *
 * @since 3.0.0
 */
enum class ViewPreset : int
{
    Top = 0,         /**< From above. */
    Perspective = 1, /**< Oblique view that the user may orbit. */
    Front = 2,       /**< From the front. */
    Left = 3,        /**< From the left. */
    Bottom = 4,      /**< From below. */
    Back = 5,        /**< From behind. */
    Right = 6        /**< From the right. */
};

/** Number of view presets. @since 3.0.0 */
inline constexpr int view_preset_count = 7;

/**
 * @param[in] preset   Preset number.
 *
 * @return @p preset clamped to the valid preset numbers.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT int clamp_view_preset(int preset);

/**
 * @param[in] preset   Preset number; out-of-range numbers are Top.
 *
 * @return The display name, e.g. "Top" or "Perspective".
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::string_view view_preset_name(int preset);

/**
 * @param[in] preset   Preset number.
 *
 * @return Whether the preset is the perspective view that orbits; the other presets pan.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT bool view_preset_is_perspective(int preset);

/**
 * @return The base orientation of the perspective view (Euler angles pitch -45, yaw -40, roll -130 degrees).
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Quaternionf perspective_preset_rotation();

/**
 * @param[in] preset   Preset number; out-of-range numbers are Top.
 *
 * @return The camera orientation of the preset.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Quaternionf view_preset_rotation(int preset);

/**
 * @param[in] axis      Rotation axis; it need not be normalised.
 * @param[in] degrees   Angle in degrees, counter-clockwise about the axis.
 *
 * @return The rotation.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Quaternionf rotation_about_axis(const Eigen::Vector3f& axis, float degrees);

/**
 * Rotation from Euler angles in degrees, applied roll (about z) first, then pitch (about x), then yaw (about y),
 * as QQuaternion::fromEulerAngles.
 *
 * @param[in] pitch   Angle about x.
 * @param[in] yaw     Angle about y.
 * @param[in] roll    Angle about z.
 *
 * @return The rotation.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Quaternionf rotation_from_euler_angles(float pitch, float yaw, float roll);

/**
 * Perspective projection to OpenGL clip space (z from -1 at the near plane to 1 at the far plane), as
 * QMatrix4x4::perspective.
 *
 * @param[in] fov_y_degrees   Vertical field of view.
 * @param[in] aspect          Width over height.
 * @param[in] near_plane      Distance of the near plane.
 * @param[in] far_plane       Distance of the far plane.
 *
 * @return The projection matrix; identity for a degenerate frustum.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Matrix4f perspective_matrix(float fov_y_degrees, float aspect, float near_plane, float far_plane);

/**
 * View matrix of a camera at @p eye looking at @p center, as QMatrix4x4::lookAt.
 *
 * @param[in] eye      Camera position.
 * @param[in] center   Point looked at.
 * @param[in] up       Up direction.
 *
 * @return The view matrix; identity when @p eye and @p center coincide.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Matrix4f look_at_matrix(const Eigen::Vector3f& eye, const Eigen::Vector3f& center,
                                                const Eigen::Vector3f& up);

/**
 * @param[in] offset   Translation.
 *
 * @return The matrix translating by @p offset.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Eigen::Matrix4f translation_matrix(const Eigen::Vector3f& offset);

/**
 * @brief Camera state of one pane of the 3-D view: preset, user rotation, zoom and pan.
 *
 * The perspective preset orbits by @c rotation (applied on top of the preset orientation); the other presets
 * pan by @c pan in the view plane. The single view uses @c rotation and @c zoom only.
 *
 * @since 3.0.0
 */
struct PaneCamera
{
    int preset = static_cast<int>(ViewPreset::Perspective);       /**< View preset number. */
    float zoom = 0.0f;                                            /**< Wheel notches zoomed in; negative zooms out. */
    Eigen::Vector2f pan = Eigen::Vector2f::Zero();                /**< Pan offset in scene units (right, up). */
    Eigen::Quaternionf rotation = Eigen::Quaternionf::Identity(); /**< User rotation on top of the preset. */
};

/**
 * @brief Projection, view and model matrices of a camera with the derived camera frame.
 *
 * @since 3.0.0
 */
struct CameraMatrices
{
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity(); /**< Perspective projection. */
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();       /**< World to eye. */
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();      /**< Moves the scene centre to the origin. */
    Eigen::Vector3f eye = Eigen::Vector3f::Zero();            /**< Camera position (relative to the scene centre). */
    Eigen::Vector3f up = Eigen::Vector3f::UnitY();            /**< Up direction. */
    Eigen::Vector3f look_at = Eigen::Vector3f::Zero();        /**< Point looked at. */
    float distance = 0.0f;                                    /**< Distance of the camera from the look-at point. */

    /** @return projection * view * model. */
    Eigen::Matrix4f pvm() const
    {
        return projection * view * model;
    }
};

/**
 * @brief Orbit camera of the 3-D brain view: looks at the scene centre from a distance set by the scene size
 * and the zoom.
 *
 * The camera sits at 1.5 scene sizes from the centre, minus 0.05 scene sizes per zoom notch, with a 45 degree
 * field of view, the near plane at 0.01 scene sizes and the far plane at 20 scene sizes but at least 100 units.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT OrbitCamera
{
public:
    //=========================================================================================================
    /**
     * Sets the centre of the scene, which the model matrix moves to the origin.
     *
     * @param[in] center   Centre.
     */
    void set_scene_center(const Eigen::Vector3f& center)
    {
        m_scene_center = center;
    }

    /** @return The centre of the scene. */
    const Eigen::Vector3f& scene_center() const
    {
        return m_scene_center;
    }

    //=========================================================================================================
    /**
     * Sets the extent of the scene; sizes of 0.01 and below are replaced by 0.3.
     *
     * @param[in] size   Extent.
     */
    void set_scene_size(float size)
    {
        m_scene_size = (size > 0.01f) ? size : 0.3f;
    }

    /** @return The extent of the scene. */
    float scene_size() const
    {
        return m_scene_size;
    }

    //=========================================================================================================
    /**
     * Matrices of the single view: the user rotation on top of the perspective preset, without pan.
     *
     * @param[in] rotation   User rotation.
     * @param[in] zoom       Zoom notches.
     * @param[in] aspect     Width over height of the pane.
     *
     * @return The matrices.
     */
    CameraMatrices single_view(const Eigen::Quaternionf& rotation, float zoom, float aspect) const;

    //=========================================================================================================
    /**
     * Matrices of a pane of the multi view: the perspective preset orbits by the pane rotation, the other
     * presets keep their orientation and pan.
     *
     * @param[in] pane     Camera state of the pane.
     * @param[in] aspect   Width over height of the pane.
     *
     * @return The matrices.
     */
    CameraMatrices pane_view(const PaneCamera& pane, float aspect) const;

    //=========================================================================================================
    /**
     * Orbits a user rotation (applied on top of the perspective preset) by a pointer drag: horizontal motion
     * turns about the camera's up axis, vertical motion about its right axis.
     *
     * @param[in] rotation            User rotation.
     * @param[in] dx                  Horizontal pointer motion in pixels.
     * @param[in] dy                  Vertical pointer motion in pixels.
     * @param[in] degrees_per_pixel   Speed.
     *
     * @return The new, normalised user rotation.
     */
    static Eigen::Quaternionf orbit(const Eigen::Quaternionf& rotation, double dx, double dy, float degrees_per_pixel = 0.5f);

    //=========================================================================================================
    /**
     * Pans by a pointer drag: 0.002 scene sizes per pixel, the scene following the pointer.
     *
     * @param[in] pan          Pan offset.
     * @param[in] dx           Horizontal pointer motion in pixels.
     * @param[in] dy           Vertical pointer motion in pixels.
     * @param[in] scene_size   Extent of the scene.
     *
     * @return The new pan offset.
     */
    static Eigen::Vector2f panned(const Eigen::Vector2f& pan, double dx, double dy, float scene_size);

private:
    CameraMatrices compute(const Eigen::Quaternionf& rotation, float zoom, const Eigen::Vector2f& pan, bool apply_pan,
                           float aspect) const;

    Eigen::Vector3f m_scene_center = Eigen::Vector3f::Zero();
    float m_scene_size = 0.3f;
};

} // namespace mne::viz

#endif // VIZ_CAMERA_H
