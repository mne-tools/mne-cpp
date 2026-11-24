//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Fit a sphere to surface points (FreeSurfer or FIFF BEM surface).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fs/fs_surface.h>
#include <mne/math/sphere.h>

#include <mne/base/log.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <fstream>
#include <print>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Fit a sphere to surface points.\n\nReads a FreeSurfer surface or FIFF BEM surface and fits a best-fit sphere.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"surf"}, "Surface file (FreeSurfer format or FIFF BEM).", "name", ""});
    parser.add_option({{"out"}, "Output text file with sphere parameters.", "name", ""});

    parser.process(argc, argv, "mne_fit_sphere_to_surf", PROGRAM_VERSION);

    const std::string surfName = parser.value("surf");
    const std::string outName = parser.value("out");

    if (surfName.empty() || outName.empty()) {
        Log::critical("Both --surf and --out are required.");
        std::print("{}", parser.help_text("mne_fit_sphere_to_surf"));
        return 1;
    }

    MatrixX3f vertices;

    // Try reading as FIFF BEM first, then as FreeSurfer surface
    bool loaded = false;

    // Attempt FIFF BEM
    if (auto bem = MNEBem::read(path_from_utf8(surfName))) {
        vertices = (*bem)[0].rr;
        Log::info("Read FIFF BEM surface: {} vertices", static_cast<int>(vertices.rows()));
        loaded = true;
    }

    // Attempt FreeSurfer surface
    if (!loaded) {
        FsSurface surf;
        if (FsSurface::read(path_from_utf8(surfName), surf, false)) {
            vertices = surf.rr();
            Log::info("Read FreeSurfer surface: {} vertices", static_cast<int>(vertices.rows()));
            loaded = true;
        }
    }

    if (!loaded) {
        Log::critical("Cannot read surface from: {}", surfName);
        return 1;
    }

    if (vertices.rows() < 4) {
        Log::critical("Need at least 4 vertices to fit a sphere, got {}.", static_cast<int>(vertices.rows()));
        return 1;
    }

    // Fit sphere using simplex method
    Log::info("Fitting sphere to {} points...", static_cast<int>(vertices.rows()));
    Sphere sphere = Sphere::fit_sphere_simplex(vertices);

    Vector3f center = sphere.center();
    float radius = sphere.radius();

    Log::info("Sphere center : {:10.6f} {:10.6f} {:10.6f} m", center(0), center(1), center(2));
    Log::info("Sphere radius : {:10.6f} m", radius);

    // Write output
    std::ofstream outFile(path_from_utf8(outName));
    if (!outFile) {
        Log::critical("Cannot open output file: {}", outName);
        return 1;
    }

    outFile << std::format("{:.8f} {:.8f} {:.8f} {:.8f}\n", center(0), center(1), center(2), radius);
    outFile.close();

    Log::info("Wrote sphere parameters to {}", outName);

    return 0;
}
