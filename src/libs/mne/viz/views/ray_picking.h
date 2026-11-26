//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ray_picking.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Pick rays from pixels and their intersections with triangles and spheres.
 */

#ifndef VIZ_RAY_PICKING_H
#define VIZ_RAY_PICKING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/geometry.h"
#include "../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief A ray in world space.
 *
 * @since 3.0.0
 */
struct Ray
{
    Eigen::Vector3f origin = Eigen::Vector3f::Zero();     /**< Start point. */
    Eigen::Vector3f direction = Eigen::Vector3f::UnitZ(); /**< Unit direction. */

    /**
     * @param[in] t   Distance along the ray.
     *
     * @return The point at distance @p t.
     */
    Eigen::Vector3f at(float t) const
    {
        return origin + t * direction;
    }
};

/**
 * @brief Where a ray enters and leaves a sphere, as distances along the ray; negative distances lie behind
 * the origin.
 *
 * @since 3.0.0
 */
struct SphereCrossing
{
    float entry = 0.0f; /**< Distance of the entry point. */
    float exit = 0.0f;  /**< Distance of the exit point. */
};

//=============================================================================================================
/**
 * The pick ray through a pixel of a pane: from the near plane to the far plane of the inverse of @p pvm.
 *
 * @param[in] pixel   Pointer position, in the coordinates of @p pane (y growing downwards).
 * @param[in] pane    Pane rectangle; widths and heights below 1 count as 1.
 * @param[in] pvm     Projection * view * model matrix of the pane (OpenGL clip space).
 *
 * @return The ray in model space, or nothing when @p pvm is not invertible.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::optional<Ray> ray_from_pixel(Point pixel, const Rect& pane, const Eigen::Matrix4f& pvm);

//=============================================================================================================
/**
 * Möller–Trumbore ray–triangle intersection in double precision; edges and vertices count as inside with a
 * tolerance of 1e-7 in barycentric coordinates.
 *
 * @param[in] ray   Ray.
 * @param[in] v0    First corner.
 * @param[in] v1    Second corner.
 * @param[in] v2    Third corner.
 *
 * @return The distance along the ray to the hit point, larger than 1e-7, or nothing on a miss.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::optional<double> intersect_ray_triangle(const Ray& ray, const Eigen::Vector3f& v0,
                                                              const Eigen::Vector3f& v1, const Eigen::Vector3f& v2);

//=============================================================================================================
/**
 * Intersection of the line of a ray with a sphere.
 *
 * @param[in] ray      Ray with a unit direction.
 * @param[in] center   Centre of the sphere.
 * @param[in] radius   Radius.
 *
 * @return Entry and exit distances, or nothing when the line misses the sphere; a tangent line enters and
 *         leaves at the same distance.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::optional<SphereCrossing> intersect_ray_sphere(const Ray& ray, const Eigen::Vector3f& center,
                                                                    float radius);

} // namespace mne::viz

#endif // VIZ_RAY_PICKING_H
