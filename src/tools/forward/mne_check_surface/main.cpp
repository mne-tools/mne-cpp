//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Check BEM surface topology (completeness, nesting, thickness).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fs/fs_surface.h>
#include <mne/base/log.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE CONSTANTS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// STATIC HELPERS (ported from SurfaceChecks in mne_surf2bem)
//=============================================================================================================

static std::string nameOf(int id)
{
    switch (id) {
        case FIFFV_BEM_SURF_ID_HEAD:
            return "outer skin ";
        case FIFFV_BEM_SURF_ID_SKULL:
            return "outer skull";
        case FIFFV_BEM_SURF_ID_BRAIN:
            return "inner skull";
        default:
            return "unknown    ";
    }
}

//=============================================================================================================

static double solidAngle(const Vector3f& from,
                         const Vector3f& v0,
                         const Vector3f& v1,
                         const Vector3f& v2)
{
    Vector3d d1 = (v0 - from).cast<double>();
    Vector3d d2 = (v1 - from).cast<double>();
    Vector3d d3 = (v2 - from).cast<double>();

    double l1 = d1.norm();
    double l2 = d2.norm();
    double l3 = d3.norm();

    double triple = d1.cross(d2).dot(d3);
    double s = l1 * l2 * l3 + d1.dot(d2) * l3 + d1.dot(d3) * l2 + d2.dot(d3) * l1;

    return 2.0 * atan2(triple, s);
}

//=============================================================================================================

static double sumSolids(const Vector3f& from, const MNEBemSurface& surf)
{
    double totAngle = 0.0;
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.itris(k, 0));
        Vector3f v1 = surf.rr.row(surf.itris(k, 1));
        Vector3f v2 = surf.rr.row(surf.itris(k, 2));
        totAngle += solidAngle(from, v0, v1, v2);
    }
    return totAngle;
}

//=============================================================================================================

static bool isCompleteSurface(const MNEBemSurface& surf)
{
    Vector3f cm = Vector3f::Zero();
    for (int k = 0; k < surf.np; ++k)
        cm += surf.rr.row(k).transpose();
    cm /= static_cast<float>(surf.np);

    Log::info("{} CM is {:6.2f} {:6.2f} {:6.2f} mm", nameOf(surf.id), 1000.0f * cm(0), 1000.0f * cm(1), 1000.0f * cm(2));

    double totAngle = sumSolids(cm, surf) / (4.0 * std::numbers::pi);
    if (fabs(totAngle - 1.0) > 1e-5) {
        Log::info("Surface {} is NOT complete (solid angle = {:g} * 4*PI instead of 1.0)", nameOf(surf.id), totAngle);
        return false;
    }
    Log::info("Surface {} is complete.", nameOf(surf.id));
    return true;
}

//=============================================================================================================

static bool checkNesting(const std::vector<MNEBemSurface>& surfs)
{
    for (std::size_t j = 0; j + 1 < surfs.size(); ++j) {
        const std::string inner = nameOf(surfs[j + 1].id);
        const std::string outer = nameOf(surfs[j].id);

        // Test first vertex of inner surface against outer
        Vector3f pt = surfs[j + 1].rr.row(0);
        double totAngle = sumSolids(pt, surfs[j]) / (4.0 * std::numbers::pi);
        if (fabs(totAngle - 1.0) > 1e-5) {
            Log::info("Checking that {} is inside {}... [FAILED]", inner, outer);
            return false;
        }
        Log::info("Checking that {} is inside {}... [OK]", inner, outer);
    }
    return true;
}

//=============================================================================================================

static void reportTriangleAreas(const MNEBemSurface& surf, const std::string& name)
{
    double minArea = 1e30, maxArea = 0.0, totalArea = 0.0;
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.itris(k, 0));
        Vector3f v1 = surf.rr.row(surf.itris(k, 1));
        Vector3f v2 = surf.rr.row(surf.itris(k, 2));
        double area = 0.5 * ((v1 - v0).cross(v2 - v0)).cast<double>().norm();
        if (area < minArea)
            minArea = area;
        if (area > maxArea)
            maxArea = area;
        totalArea += area;
    }
    Log::info("{}: {} triangles, area {:7.1f} ... {:7.1f} mm^2 (total {:10.1f} mm^2)", name, surf.ntri, 1e6 * minArea, 1e6 * maxArea, 1e6 * totalArea);
}

//=============================================================================================================

static float minSurfaceDist(const MNEBemSurface& s1, const MNEBemSurface& s2)
{
    float minDist = 1e30f;
    for (int k1 = 0; k1 < s1.np; ++k1) {
        for (int k2 = 0; k2 < s2.np; ++k2) {
            float dist = (s1.rr.row(k1) - s2.rr.row(k2)).norm();
            if (dist < minDist)
                minDist = dist;
        }
    }
    return minDist;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Check BEM surface topology (completeness, nesting, thickness).\n\nUse --bem for BEM FIFF files or --surf for FreeSurfer surface files.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"bem"}, "BEM FIFF file to check.", "file", ""});
    parser.add_option({{"surf"}, "FreeSurfer surface file to check (standalone).", "file", ""});
    parser.add_option({{"id"}, "Surface ID for --surf: 4=head, 3=skull, 1=brain.", "id", "1"});
    parser.add_option({{"thickness"}, "Also check inter-surface distances.", "", ""});

    parser.process(argc, argv, "mne_check_surface", PROGRAM_VERSION);

    const std::string bemFile = parser.value("bem");
    const std::string surfFile = parser.value("surf");
    const int surfId = to_int(parser.value("id")).value_or(0);
    const bool doThickness = parser.is_set("thickness");

    if (bemFile.empty() && surfFile.empty()) {
        Log::critical("Either --bem or --surf is required.");
        return 1;
    }

    std::vector<MNEBemSurface> surfs;

    if (!bemFile.empty()) {
        // Read BEM from FIFF
        MNEBem bem(path_from_utf8(bemFile));
        if (bem.size() == 0) {
            Log::critical("Cannot read BEM from: {}", bemFile);
            return 1;
        }
        Log::info("Read {} surfaces from {}\n", bem.size(), bemFile);
        for (int i = 0; i < std::ssize(bem); ++i)
            surfs.push_back(bem[i]);
    } else {
        // Read single FreeSurfer surface
        FsSurface surface;
        if (!FsSurface::read(path_from_utf8(surfFile), surface)) {
            Log::critical("Cannot read surface: {}", surfFile);
            return 1;
        }
        MNEBemSurface bemSurf;
        bemSurf.id = surfId;
        bemSurf.np = static_cast<int>(surface.rr().rows());
        bemSurf.ntri = static_cast<int>(surface.tris().rows());
        bemSurf.rr = surface.rr();
        bemSurf.itris = surface.tris();
        bemSurf.nn = surface.nn();
        bemSurf.coord_frame = FIFFV_COORD_MRI;
        surfs.push_back(bemSurf);
        Log::info("Read surface: {} ({} vertices, {} triangles)\n", surfFile, bemSurf.np, bemSurf.ntri);
    }

    // Check completeness
    bool allOk = true;
    for (std::size_t k = 0; k < surfs.size(); ++k) {
        if (!isCompleteSurface(surfs[k]))
            allOk = false;
        reportTriangleAreas(surfs[k], nameOf(surfs[k].id));
    }
    Log::info("{}", "");

    // Check nesting if multiple surfaces
    if (surfs.size() > 1) {
        if (!checkNesting(surfs))
            allOk = false;
    }

    // Check inter-surface distances
    if (doThickness && surfs.size() > 1) {
        Log::info("{}", "");
        for (std::size_t k = 0; k + 1 < surfs.size(); ++k) {
            float dist = minSurfaceDist(surfs[k], surfs[k + 1]);
            Log::info("Minimum distance between {} and {}: {:6.1f} mm", nameOf(surfs[k].id), nameOf(surfs[k + 1].id), 1000.0f * dist);
        }
    }

    if (allOk)
        Log::info("\nAll surface checks passed.");
    else
        Log::info("\nSome surface checks FAILED.");

    return allOk ? 0 : 1;
}
