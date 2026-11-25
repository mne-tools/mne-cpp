//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     camera.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of OrbitCamera, the view presets and the matrix helpers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "camera.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <numbers>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

namespace
{

constexpr float radians(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

} // namespace

//=============================================================================================================

int clamp_view_preset(int preset)
{
    return std::clamp(preset, 0, view_preset_count - 1);
}

//=============================================================================================================

std::string_view view_preset_name(int preset)
{
    switch (preset) {
        case 1:
            return "Perspective";
        case 2:
            return "Front";
        case 3:
            return "Left";
        case 4:
            return "Bottom";
        case 5:
            return "Back";
        case 6:
            return "Right";
        default:
            return "Top";
    }
}

//=============================================================================================================

bool view_preset_is_perspective(int preset)
{
    return preset == static_cast<int>(ViewPreset::Perspective);
}

//=============================================================================================================

Eigen::Quaternionf rotation_about_axis(const Eigen::Vector3f& axis, float degrees)
{
    const float norm = axis.norm();
    if (norm <= 0.0f) {
        return Eigen::Quaternionf::Identity();
    }
    return Eigen::Quaternionf(Eigen::AngleAxisf(radians(degrees), axis / norm)).normalized();
}

//=============================================================================================================

Eigen::Quaternionf rotation_from_euler_angles(float pitch, float yaw, float roll)
{
    // Half angles; the product yaw(y) * pitch(x) * roll(z) written out
    const float c1 = std::cos(radians(yaw) * 0.5f);
    const float s1 = std::sin(radians(yaw) * 0.5f);
    const float c2 = std::cos(radians(roll) * 0.5f);
    const float s2 = std::sin(radians(roll) * 0.5f);
    const float c3 = std::cos(radians(pitch) * 0.5f);
    const float s3 = std::sin(radians(pitch) * 0.5f);
    const float c1c2 = c1 * c2;
    const float s1s2 = s1 * s2;
    return {c1c2 * c3 + s1s2 * s3, c1c2 * s3 + s1s2 * c3, s1 * c2 * c3 - c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3};
}

//=============================================================================================================

Eigen::Quaternionf perspective_preset_rotation()
{
    return rotation_from_euler_angles(-45.0f, -40.0f, -130.0f);
}

//=============================================================================================================

Eigen::Quaternionf view_preset_rotation(int preset)
{
    const Eigen::Vector3f x = Eigen::Vector3f::UnitX();
    const Eigen::Vector3f y = Eigen::Vector3f::UnitY();
    const Eigen::Vector3f z = Eigen::Vector3f::UnitZ();
    switch (preset) {
        case 1:
            return perspective_preset_rotation();
        case 2:
            return rotation_about_axis(y, 180.0f) * rotation_about_axis(z, 180.0f) * rotation_about_axis(x, 90.0f) *
                rotation_about_axis(z, 180.0f);
        case 3:
            return rotation_about_axis(x, 90.0f) * rotation_about_axis(y, -90.0f);
        case 4:
            return rotation_about_axis(x, 180.0f) * rotation_about_axis(z, 180.0f);
        case 5:
            return rotation_about_axis(x, 90.0f);
        case 6:
            return rotation_about_axis(x, 90.0f) * rotation_about_axis(y, 90.0f);
        default:
            return rotation_about_axis(z, 180.0f);
    }
}

//=============================================================================================================

Eigen::Matrix4f perspective_matrix(float fov_y_degrees, float aspect, float near_plane, float far_plane)
{
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    if (near_plane == far_plane || aspect == 0.0f) {
        return m;
    }
    const float half = radians(fov_y_degrees / 2.0f);
    const float sine = std::sin(half);
    if (sine == 0.0f) {
        return m;
    }
    const float cotan = std::cos(half) / sine;
    const float clip = far_plane - near_plane;
    m(0, 0) = cotan / aspect;
    m(1, 1) = cotan;
    m(2, 2) = -(near_plane + far_plane) / clip;
    m(2, 3) = -(2.0f * near_plane * far_plane) / clip;
    m(3, 2) = -1.0f;
    m(3, 3) = 0.0f;
    return m;
}

//=============================================================================================================

Eigen::Matrix4f look_at_matrix(const Eigen::Vector3f& eye, const Eigen::Vector3f& center, const Eigen::Vector3f& up)
{
    Eigen::Vector3f forward = center - eye;
    constexpr float fuzzy = 0.00001f;
    if (std::abs(forward.x()) <= fuzzy && std::abs(forward.y()) <= fuzzy && std::abs(forward.z()) <= fuzzy) {
        return Eigen::Matrix4f::Identity();
    }
    forward.normalize();
    const Eigen::Vector3f side = forward.cross(up).normalized();
    const Eigen::Vector3f true_up = side.cross(forward);

    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.block<1, 3>(0, 0) = side.transpose();
    m.block<1, 3>(1, 0) = true_up.transpose();
    m.block<1, 3>(2, 0) = -forward.transpose();
    return m * translation_matrix(-eye);
}

//=============================================================================================================

Eigen::Matrix4f translation_matrix(const Eigen::Vector3f& offset)
{
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.block<3, 1>(0, 3) = offset;
    return m;
}

//=============================================================================================================

CameraMatrices OrbitCamera::compute(const Eigen::Quaternionf& rotation, float zoom, const Eigen::Vector2f& pan,
                                    bool apply_pan, float aspect) const
{
    CameraMatrices result;
    const float far_plane = std::max(m_scene_size * 20.0f, 100.0f);
    result.projection = perspective_matrix(45.0f, aspect, m_scene_size * 0.01f, far_plane);

    const float base_distance = m_scene_size * 1.5f;
    result.distance = base_distance - zoom * (m_scene_size * 0.05f);
    result.eye = rotation * Eigen::Vector3f(0.0f, 0.0f, result.distance);
    result.up = rotation * Eigen::Vector3f::UnitY();
    result.look_at = Eigen::Vector3f::Zero();

    // Pan shifts camera and look-at point along the view plane
    if (apply_pan && (pan.x() != 0.0f || pan.y() != 0.0f)) {
        const Eigen::Vector3f right = (rotation * Eigen::Vector3f::UnitX()).normalized();
        const Eigen::Vector3f offset = right * pan.x() + result.up.normalized() * pan.y();
        result.look_at += offset;
        result.eye += offset;
    }

    result.view = look_at_matrix(result.eye, result.look_at, result.up);
    result.model = translation_matrix(-m_scene_center);
    return result;
}

//=============================================================================================================

CameraMatrices OrbitCamera::single_view(const Eigen::Quaternionf& rotation, float zoom, float aspect) const
{
    return compute(rotation * perspective_preset_rotation(), zoom, Eigen::Vector2f::Zero(), false, aspect);
}

//=============================================================================================================

CameraMatrices OrbitCamera::pane_view(const PaneCamera& pane, float aspect) const
{
    const int preset = clamp_view_preset(pane.preset);
    const Eigen::Quaternionf offset = view_preset_rotation(preset);
    const bool perspective = view_preset_is_perspective(preset);
    return compute(perspective ? Eigen::Quaternionf(pane.rotation * offset) : offset, pane.zoom, pane.pan, !perspective,
                   aspect);
}

//=============================================================================================================

Eigen::Quaternionf OrbitCamera::orbit(const Eigen::Quaternionf& rotation, double dx, double dy, float degrees_per_pixel)
{
    const Eigen::Quaternionf preset = perspective_preset_rotation();
    const Eigen::Quaternionf effective = rotation * preset;

    const Eigen::Vector3f up_axis = (effective * Eigen::Vector3f::UnitY()).normalized();
    const Eigen::Vector3f right_axis = (effective * Eigen::Vector3f::UnitX()).normalized();

    const Eigen::Quaternionf yaw = rotation_about_axis(up_axis, static_cast<float>(-dx) * degrees_per_pixel);
    const Eigen::Quaternionf pitch = rotation_about_axis(right_axis, static_cast<float>(-dy) * degrees_per_pixel);

    return (yaw * pitch * effective * preset.conjugate()).normalized();
}

//=============================================================================================================

Eigen::Vector2f OrbitCamera::panned(const Eigen::Vector2f& pan, double dx, double dy, float scene_size)
{
    const float speed = scene_size * 0.002f;
    return pan + Eigen::Vector2f(static_cast<float>(-dx) * speed, static_cast<float>(dy) * speed);
}

} // namespace mne::scene
