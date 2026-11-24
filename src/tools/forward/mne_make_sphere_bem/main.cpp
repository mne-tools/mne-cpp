//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Create a spherical BEM model by tessellating concentric spheres.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_constants.h>
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

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

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
/**
 * Create an icosphere by recursively subdividing an icosahedron.
 * Returns vertex positions (unit sphere) and triangle indices.
 */
static void makeIcosphere(int nSubdiv, MatrixX3f& verts, MatrixX3i& tris)
{
    // Initial icosahedron
    float t = (1.0f + sqrtf(5.0f)) / 2.0f;

    std::vector<Vector3f> v = {
        Vector3f(-1, t, 0).normalized(), Vector3f(1, t, 0).normalized(),
        Vector3f(-1, -t, 0).normalized(), Vector3f(1, -t, 0).normalized(),
        Vector3f(0, -1, t).normalized(), Vector3f(0, 1, t).normalized(),
        Vector3f(0, -1, -t).normalized(), Vector3f(0, 1, -t).normalized(),
        Vector3f(t, 0, -1).normalized(), Vector3f(t, 0, 1).normalized(),
        Vector3f(-t, 0, -1).normalized(), Vector3f(-t, 0, 1).normalized()};

    std::vector<Vector3i> f = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};

    // Subdivision
    for (int s = 0; s < nSubdiv; ++s) {
        std::vector<Vector3i> newFaces;
        std::map<std::pair<int, int>, int> edgeMidpoint;

        auto getMidpoint = [&](int a, int b) -> int {
            auto key = std::make_pair(std::min(a, b), std::max(a, b));
            auto it = edgeMidpoint.find(key);
            if (it != edgeMidpoint.end())
                return it->second;
            int idx = static_cast<int>(v.size());
            v.push_back(((v[a] + v[b]) * 0.5f).normalized());
            edgeMidpoint[key] = idx;
            return idx;
        };

        for (const auto& face : f) {
            int a = getMidpoint(face(0), face(1));
            int b = getMidpoint(face(1), face(2));
            int c = getMidpoint(face(2), face(0));
            newFaces.push_back(Vector3i(face(0), a, c));
            newFaces.push_back(Vector3i(face(1), b, a));
            newFaces.push_back(Vector3i(face(2), c, b));
            newFaces.push_back(Vector3i(a, b, c));
        }
        f = newFaces;
    }

    verts.resize(v.size(), 3);
    for (int i = 0; i < static_cast<int>(v.size()); ++i)
        verts.row(i) = v[i].transpose();

    tris.resize(f.size(), 3);
    for (int i = 0; i < static_cast<int>(f.size()); ++i)
        tris.row(i) = f[i].transpose();
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Create a spherical BEM model.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"out"}, "Output BEM FIFF file.", "file", ""});
    parser.add_option({{"origin"}, "Sphere origin in mm (x,y,z).", "x,y,z", "0,0,40"});
    parser.add_option({{"radii"}, "Shell radii in mm.", "r1,r2,r3", "71,80,90"});
    parser.add_option({{"conductivity"}, "Conductivities.", "s1,s2,s3", "0.33,0.0042,0.33"});
    parser.add_option({{"ico"}, "Icosahedron subdivision level.", "n", "4"});

    parser.process(argc, argv, "mne_make_sphere_bem", PROGRAM_VERSION);

    const std::string outFile = parser.value("out");
    const auto toFloat = [](const std::string& text) {
        return static_cast<float>(to_double(text).value_or(0.0));
    };

    // Parse origin
    const StringList originParts = split(parser.value("origin"), ',');
    if (originParts.size() != 3) {
        Log::critical("--origin: need x,y,z");
        return 1;
    }
    Vector3f origin(toFloat(originParts[0]) / 1000.0f,
                    toFloat(originParts[1]) / 1000.0f,
                    toFloat(originParts[2]) / 1000.0f);

    // Parse radii
    std::vector<float> radii;
    for (const std::string& p : split(parser.value("radii"), ','))
        radii.push_back(toFloat(p) / 1000.0f);

    // Parse conductivities
    std::vector<float> sigmas;
    for (const std::string& p : split(parser.value("conductivity"), ','))
        sigmas.push_back(toFloat(p));

    const int icoLevel = to_int(parser.value("ico")).value_or(0);

    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }
    if (radii.size() != sigmas.size()) {
        Log::critical("Number of radii ({}) must match number of conductivities ({})", static_cast<long long>(radii.size()), static_cast<long long>(sigmas.size()));
        return 1;
    }

    // Create icosphere template
    MatrixX3f unitVerts;
    MatrixX3i tris;
    makeIcosphere(icoLevel, unitVerts, tris);
    Log::info("Icosphere subdivision {}: {} vertices, {} triangles", icoLevel, static_cast<int>(unitVerts.rows()), static_cast<int>(tris.rows()));

    // Create BEM model
    MNEBem bem;
    const std::vector<int> surfIds = {FIFFV_BEM_SURF_ID_BRAIN, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_HEAD};

    // Sort radii ascending (innermost first)
    std::vector<std::size_t> sortIdx(radii.size());
    for (std::size_t i = 0; i < radii.size(); ++i)
        sortIdx[i] = i;
    std::sort(sortIdx.begin(), sortIdx.end(), [&](std::size_t a, std::size_t b) { return radii[a] < radii[b]; });

    for (std::size_t i = 0; i < radii.size(); ++i) {
        const std::size_t si = sortIdx[i];
        float radius = radii[si];
        float sigma = sigmas[si];
        int surfId = (i < surfIds.size()) ? surfIds[i] : FIFFV_BEM_SURF_ID_BRAIN;

        MNEBemSurface surf;
        surf.id = surfId;
        surf.np = static_cast<int>(unitVerts.rows());
        surf.ntri = static_cast<int>(tris.rows());
        surf.coord_frame = FIFFV_COORD_HEAD;
        surf.sigma = sigma;

        // Scale vertices to radius and translate to origin
        surf.rr = unitVerts * radius;
        for (int v = 0; v < surf.np; ++v) {
            surf.rr(v, 0) += origin(0);
            surf.rr(v, 1) += origin(1);
            surf.rr(v, 2) += origin(2);
        }

        surf.itris = tris;
        surf.nn = unitVerts; // normals point outward on unit sphere

        Log::info("Surface {} ({}): radius={:6.1f} mm, sigma={:g} S/m, {} vertices", i + 1, MNEBemSurface::id_name(surfId), 1000.0f * radius, sigma, surf.np);

        bem.push_back(surf);
    }

    // Write BEM
    if (auto written = bem.write(path_from_utf8(outFile)); !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }
    Log::info("Written spherical BEM model to: {}", outFile);

    return 0;
}
