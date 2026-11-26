//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ray_picking.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the pick ray and the ray intersections.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ray_picking.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>
#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

std::optional<Ray> ray_from_pixel(Point pixel, const Rect& pane, const Eigen::Matrix4f& pvm)
{
    const Eigen::Matrix4d matrix = pvm.cast<double>();
    const double determinant = matrix.determinant();
    if (!std::isfinite(determinant) || std::abs(determinant) <= 1e-12) {
        return std::nullopt;
    }
    const Eigen::Matrix4d inverse = matrix.inverse();

    const double width = std::max(1.0, pane.width);
    const double height = std::max(1.0, pane.height);
    const double ndc_x = 2.0 * (pixel.x - pane.x) / width - 1.0;
    const double ndc_y = 1.0 - 2.0 * (pixel.y - pane.y) / height;

    Eigen::Vector4d near_point = inverse * Eigen::Vector4d(ndc_x, ndc_y, -1.0, 1.0);
    Eigen::Vector4d far_point = inverse * Eigen::Vector4d(ndc_x, ndc_y, 1.0, 1.0);
    near_point /= near_point.w();
    far_point /= far_point.w();

    Ray ray;
    ray.origin = near_point.head<3>().cast<float>();
    ray.direction = (far_point.head<3>() - near_point.head<3>()).normalized().cast<float>();
    return ray;
}

//=============================================================================================================

std::optional<double> intersect_ray_triangle(const Ray& ray, const Eigen::Vector3f& v0, const Eigen::Vector3f& v1,
                                             const Eigen::Vector3f& v2)
{
    const Eigen::Vector3d origin = ray.origin.cast<double>();
    const Eigen::Vector3d direction = ray.direction.cast<double>();
    const Eigen::Vector3d a = v0.cast<double>();
    const Eigen::Vector3d edge1 = v1.cast<double>() - a;
    const Eigen::Vector3d edge2 = v2.cast<double>() - a;

    const Eigen::Vector3d h = direction.cross(edge2);
    const double det = edge1.dot(h);
    if (std::abs(det) < 1e-18) {
        return std::nullopt; // Parallel to the triangle
    }

    const double f = 1.0 / det;
    const Eigen::Vector3d s = origin - a;
    const double u = f * s.dot(h);
    if (u < -1e-7 || u > 1.0000001) {
        return std::nullopt;
    }

    const Eigen::Vector3d q = s.cross(edge1);
    const double v = f * direction.dot(q);
    if (v < -1e-7 || u + v > 1.0000001) {
        return std::nullopt;
    }

    const double t = f * edge2.dot(q);
    if (t > 1e-7) {
        return t;
    }
    return std::nullopt;
}

//=============================================================================================================

std::optional<SphereCrossing> intersect_ray_sphere(const Ray& ray, const Eigen::Vector3f& center, float radius)
{
    const Eigen::Vector3f to_center = center - ray.origin;
    const float closest = to_center.dot(ray.direction);
    const float distance_sq = to_center.squaredNorm() - closest * closest;
    const float radius_sq = radius * radius;
    if (distance_sq > radius_sq) {
        return std::nullopt;
    }
    const float half_chord = std::sqrt(radius_sq - distance_sq);
    return SphereCrossing{closest - half_chord, closest + half_chord};
}

} // namespace mne::viz
