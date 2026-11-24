//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutmaker.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     November 2014
 * @brief    Implementation of @ref mne::LayoutMaker — sphere fit, azimuthal projection and @c .lout writer.
 *
 * The sphere is fitted with @ref mne::Sphere::fit_sphere_simplex,
 * the Nelder-Mead fit of the original MNE-C @c make_eeg_layout routine.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutmaker.h"

#include <mne/base/log.h>
#include <mne/math/sphere.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <format>
#include <fstream>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

// Polar angle and azimuth of a direction, as MNE-C's sphere_coord: on the z axis both are zero.
Vector2d polar_azimuth(const Vector3d& r)
{
    const double rxy = std::hypot(r.x(), r.y());
    if (rxy < 1e-6)
        return Vector2d::Zero();
    const double phi = std::atan2(r.y(), r.x());
    return {std::acos(r.z() / r.norm()), phi < 0.0 ? phi + 2.0 * std::numbers::pi : phi};
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<std::vector<Vector2d>> LayoutMaker::make_layout(std::span<const Vector3d> positions,
                                                              const LayoutOptions& options)
{
    if (positions.empty()) {
        Log::warning("No positions to lay out.");
        return std::nullopt;
    }

    Vector3d origin = Vector3d::Zero();
    if (options.fit_sphere) {
        MatrixX3f points(static_cast<Index>(positions.size()), 3);
        for (Index k = 0; k < points.rows(); ++k)
            points.row(k) = positions[static_cast<std::size_t>(k)].cast<float>().transpose();
        const Sphere sphere = Sphere::fit_sphere_simplex(points, 0.05);
        if (sphere.radius() > 0.0f) {
            origin = sphere.center().cast<double>();
            Log::info("Best-fitting sphere: origin ({}, {}, {}), radius {}", origin.x(), origin.y(), origin.z(), sphere.radius());
        } else {
            // As MNE-C mne_make_eeg_layout: a failed fit falls back to the origin
            Log::warning("Sphere fitting failed, using the origin");
        }
    }

    // Azimuthal equidistant projection: the polar angle becomes the distance from the centre
    std::vector<Vector2d> centres;
    centres.reserve(positions.size());
    for (const Vector3d& position : positions) {
        const Vector2d angles = polar_azimuth(position - origin);
        const double distance = options.radius * 2.0 * angles.x() / std::numbers::pi;
        centres.emplace_back(distance * std::cos(angles.y()), distance * std::sin(angles.y()));
    }

    Vector2d minimum = centres.front();
    Vector2d maximum = minimum;
    for (const Vector2d& centre : centres) {
        minimum = minimum.cwiseMin(centre);
        maximum = maximum.cwiseMax(centre);
    }
    if (minimum.x() == maximum.x() || minimum.y() == maximum.y()) {
        Log::critical("Cannot make a layout. All positions are identical");
        return std::nullopt;
    }

    const Vector2d halfBox(0.5 * options.box_width, 0.5 * options.box_height);
    const Vector2d mirror(options.mirror_x ? -1.0 : 1.0, options.mirror_y ? -1.0 : 1.0);
    for (Vector2d& centre : centres)
        centre = (centre - halfBox).cwiseProduct(mirror);
    return centres;
}

//=============================================================================================================

bool LayoutMaker::write_lout(const std::filesystem::path& path,
                             std::span<const Vector2d> corners,
                             const StringList& names,
                             const LayoutOptions& options)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        Log::critical("Cannot open the layout file {}", path.string());
        return false;
    }
    file << "0.000000 0.000000 0.000000 0.000000\n";
    for (std::size_t k = 0; k < corners.size(); ++k) {
        file << std::format("{} {:.6f} {:.6f} {:.6f} {:.6f}", k + 1, corners[k].x(), corners[k].y(), options.box_width, options.box_height);
        if (k < names.size())
            file << ' ' << names[k];
        file << '\n';
    }
    return static_cast<bool>(file);
}
