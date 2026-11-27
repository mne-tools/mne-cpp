//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     instancing.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Model matrices and colours of instanced brain-view glyphs (spheres, cones, cylinders).
 */

#ifndef VIZ_INSTANCING_H
#define VIZ_INSTANCING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/style.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstring>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * Shortest rotation that turns @p from into @p to.
 *
 * Opposite directions rotate by half a turn about an axis perpendicular to @p from, the x axis
 * when possible, as QQuaternion::rotationTo does, so glyphs keep their orientation across backends.
 *
 * @param[in] from   Start direction, any length.
 * @param[in] to     Target direction, any length.
 * @return The unit rotation.
 * @since 3.0.0
 */
inline Eigen::Quaternionf rotationTo(const Eigen::Vector3f& from, const Eigen::Vector3f& to)
{
    const Eigen::Vector3f v0 = from.normalized();
    const Eigen::Vector3f v1 = to.normalized();
    float d = v0.dot(v1) + 1.0f;
    if (std::abs(d) <= 1e-5f) {
        Eigen::Vector3f axis = Eigen::Vector3f::UnitX().cross(v0);
        if (axis.squaredNorm() <= 1e-5f) {
            axis = Eigen::Vector3f::UnitY().cross(v0);
        }
        axis.normalize();
        return {0.0f, axis.x(), axis.y(), axis.z()};
    }
    d = std::sqrt(2.0f * d);
    const Eigen::Vector3f axis = v0.cross(v1) / d;
    return Eigen::Quaternionf(d * 0.5f, axis.x(), axis.y(), axis.z()).normalized();
}

//=============================================================================================================
/**
 * Model matrix that scales, then rotates, then translates.
 *
 * @param[in] translation   Position.
 * @param[in] rotation      Orientation.
 * @param[in] scale         Scale per axis.
 * @return The column-major model matrix.
 * @since 3.0.0
 */
inline Eigen::Matrix4f modelMatrix(const Eigen::Vector3f& translation, const Eigen::Quaternionf& rotation, const Eigen::Vector3f& scale)
{
    Eigen::Affine3f model = Eigen::Affine3f::Identity();
    model.translate(translation);
    model.rotate(rotation);
    model.scale(scale);
    return model.matrix();
}

//=============================================================================================================
/**
 * Model matrix of a unit cylinder along y (height 1, radius 1) stretched from @p from to @p to.
 *
 * @param[in] from     Start of the axis.
 * @param[in] to       End of the axis; must differ from @p from.
 * @param[in] radius   Radius.
 * @return The column-major model matrix.
 * @since 3.0.0
 */
inline Eigen::Matrix4f segmentMatrix(const Eigen::Vector3f& from, const Eigen::Vector3f& to, float radius)
{
    const Eigen::Vector3f axis = to - from;
    const float length = axis.norm();
    return modelMatrix((from + to) * 0.5f, rotationTo(Eigen::Vector3f::UnitY(), axis / length), Eigen::Vector3f(radius, length, radius));
}

//=============================================================================================================
/**
 * Copy a model matrix into the 16 floats of an instance record.
 *
 * @param[in] model   Column-major model matrix.
 * @param[out] out    16 floats, column-major as the shaders expect.
 * @since 3.0.0
 */
inline void storeMatrix(const Eigen::Matrix4f& model, float* out)
{
    std::memcpy(out, model.data(), 16 * sizeof(float));
}

//=============================================================================================================
/**
 * Copy a colour into the four floats of an instance record.
 *
 * @param[in] color   The colour.
 * @param[out] out    Red, green, blue and opacity in [0, 1].
 * @since 3.0.0
 */
inline void storeColor(const Color& color, float* out)
{
    out[0] = static_cast<float>(color.r) / 255.0f;
    out[1] = static_cast<float>(color.g) / 255.0f;
    out[2] = static_cast<float>(color.b) / 255.0f;
    out[3] = static_cast<float>(color.a) / 255.0f;
}

} // namespace mne::viz

#endif // VIZ_INSTANCING_H
