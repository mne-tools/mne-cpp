//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surf2bem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Surf2Bem class definition.
 *
 *           Ported from the original MNE C tool mne_surf2bem by Matti Hamalainen
 *           (SVN $Id: mne_surf2bem.c 3351 2012-03-05 12:03:50Z msh $).
 *
 *           Cross-referenced with MNE-Python's mne.write_bem_surfaces().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "surf2bem.h"
#include "mne_surf2bem_settings.h"
#include "surfacechecks.h"

#include <mne/fs/fs_surface.h>

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>

#include <mne/fiff/fiff_file.h>

#include <mne/fiff/fiff_byte_swap.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <format>
#include <fstream>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Surf2Bem::Surf2Bem(const MNESurf2BemSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int Surf2Bem::run()
{
    const std::vector<SurfaceInput>& inputs = m_settings.surfaces();

    if (inputs.empty()) {
        Log::info("Usage: mne_surf2bem [options]");
        Log::info("Use --help for available options.");
        return 1;
    }

    //
    // Report input summary
    //
    Log::info("{}", "");
    for (std::size_t k = 0; k < inputs.size(); ++k) {
        std::string line = std::format("input  file # {:3d} : {}", k + 1, inputs[k].fileName);
        line += inputs[k].id > 0 ? std::format(" / id = {}", inputs[k].id) : std::string(" / id N/A");
        line += inputs[k].sigma > 0.0f ? std::format(" / sigma = {:g} S/m", static_cast<double>(inputs[k].sigma)) : std::string(" / sigma N/A");
        if (inputs[k].shift != 0.0f)
            line += std::format(" / shift {:6.1f} mm", static_cast<double>(1000.0f * inputs[k].shift));
        Log::info("{}", line);
    }
    if (!m_settings.outputFile().empty())
        Log::info("output file       : {}", m_settings.outputFile());
    Log::info("{}", "");

    //
    // Step 1: Read all surfaces
    //
    std::vector<MNEBemSurface> surfs(inputs.size());

    for (std::size_t k = 0; k < inputs.size(); ++k) {
        bool ok;
        if (inputs[k].isAsciiTri) {
            ok = readAsciiTriSurface(inputs[k], surfs[k]);
            if (ok) {
                surfs[k].coord_frame = m_settings.coordFrame();
            }
        } else {
            ok = readFreeSurferSurface(inputs[k], surfs[k]);
        }

        if (!ok) {
            Log::critical("Failed to read surface {}", inputs[k].fileName);
            fflush(stderr);
            return 1;
        }

        surfs[k].id = inputs[k].id;
        Log::info("{} read. id = {}\n", inputs[k].fileName, inputs[k].id);
        fflush(stderr);
    }

    //
    // Step 2: Shift vertices along normals if requested
    //
    for (std::size_t k = 0; k < inputs.size(); ++k) {
        if (inputs[k].shift != 0.0f) {
            if (!shiftVertices(surfs[k], inputs[k].shift)) {
                return 1;
            }
        }
    }

    //
    // Step 3: Order surfaces for topology checks (head > skull > brain)
    //
    orderSurfaces(surfs);

    //
    // Step 4: Topology checks
    //
    if (m_settings.check() && (surfs.size() == 3 || surfs.size() == 1)) {
        if (!SurfaceChecks::checkSurfaces(surfs)) {
            return 1;
        }
        for (std::size_t k = 0; k < surfs.size(); ++k) {
            if (!SurfaceChecks::checkSurfaceSize(surfs[k])) {
                return 1;
            }
        }
        Log::info("Surfaces passed the basic topology checks.");

        if (m_settings.checkMore()) {
            if (!SurfaceChecks::checkThicknesses(surfs)) {
                return 1;
            }
        }
    } else {
        Log::info("Topology checks skipped.");
    }

    //
    // Step 5: Write BEM FIFF output
    //
    if (!m_settings.outputFile().empty()) {
        // Build MNEBem from surfaces, applying conductivities
        MNEBem bem;
        for (std::size_t k = 0; k < surfs.size(); ++k) {
            if (inputs[k].sigma > 0.0f) {
                surfs[k].sigma = inputs[k].sigma;
            }
            bem.push_back(surfs[k]);
        }


        if (auto written = bem.write(path_from_utf8(m_settings.outputFile())); !written) {
            Log::critical("{}", written.error().what());


            return 1;
        }

        Log::info("{} written.", m_settings.outputFile());
    } else {
        Log::info("No output requested.");
    }

    return 0;
}

//=============================================================================================================

bool Surf2Bem::readFreeSurferSurface(const SurfaceInput& input, MNEBemSurface& bemSurf)
{
    //
    // Read FreeSurfer binary surface directly.
    // BEM surface names such as inner_skull.surf name no hemisphere, which FsSurface::read() requires
    auto geometry = FsSurface::read_geometry(path_from_utf8(input.fileName));
    if (!geometry) {
        Log::critical("{}", geometry.error().what());
        fflush(stderr);
        return false;
    }
    Log::info("{}: {} vertices and {} triangles", input.fileName, static_cast<int>(geometry->rr.rows()), static_cast<int>(geometry->tris.rows()));

    bemSurf.np = static_cast<int>(geometry->rr.rows());
    bemSurf.ntri = static_cast<int>(geometry->tris.rows());
    bemSurf.coord_frame = FIFFV_COORD_MRI;
    bemSurf.rr = geometry->rr;
    bemSurf.itris = geometry->tris;

    // Compute vertex normals
    bemSurf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(bemSurf.rr), Eigen::MatrixX3i(bemSurf.itris));

    Log::info("Read FreeSurfer surface: {} vertices, {} triangles", bemSurf.np, bemSurf.ntri);
    fflush(stderr);

    return true;
}

//=============================================================================================================

bool Surf2Bem::readAsciiTriSurface(const SurfaceInput& input, MNEBemSurface& bemSurf)
{
    //
    // Read ASCII triangle file
    //
    // Format:
    //   nvert
    //   x1 y1 z1
    //   x2 y2 z2
    //   ...
    //   ntri
    //   v1 v2 v3   (1-based vertex indices)
    //   ...
    //
    std::ifstream in(path_from_utf8(input.fileName));
    if (!in) {
        Log::critical("Could not open ASCII triangle file: {}", input.fileName);
        return false;
    }

    // Read number of vertices
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        Log::critical("Invalid vertex count in {}", input.fileName);
        return false;
    }

    // Read vertices
    MatrixX3f rr(nvert, 3);
    for (int k = 0; k < nvert; ++k) {
        // Read as double and round to float, as QTextStream did
        double x = 0.0, y = 0.0, z = 0.0;
        in >> x >> y >> z;
        if (!in) {
            Log::critical("Error reading vertex {} from {}", k, input.fileName);
            return false;
        }
        rr(k, 0) = static_cast<float>(x);
        rr(k, 1) = static_cast<float>(y);
        rr(k, 2) = static_cast<float>(z);
    }

    // Read number of triangles
    int ntri = 0;
    in >> ntri;
    if (ntri <= 0) {
        Log::critical("Invalid triangle count in {}", input.fileName);
        return false;
    }

    // Read triangles (1-based in file, convert to 0-based)
    MatrixX3i tris(ntri, 3);
    for (int k = 0; k < ntri; ++k) {
        int v1 = 0, v2 = 0, v3 = 0;
        in >> v1 >> v2 >> v3;
        if (!in) {
            Log::critical("Error reading triangle {} from {}", k, input.fileName);
            return false;
        }
        if (input.swap) {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v3 - 1; // Swapped
            tris(k, 2) = v2 - 1; // Swapped
        } else {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v2 - 1;
            tris(k, 2) = v3 - 1;
        }
    }
    in.close();

    //
    // Convert units: if mm==true (default), convert from mm to meters
    //
    if (input.mm) {
        rr /= 1000.0f;
    }

    //
    // Populate BEM surface
    //
    bemSurf.np = nvert;
    bemSurf.ntri = ntri;
    bemSurf.rr = rr;
    bemSurf.itris = tris;

    // Compute vertex normals
    bemSurf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(bemSurf.rr), Eigen::MatrixX3i(bemSurf.itris));

    Log::info("Read ASCII triangle surface: {} vertices, {} triangles", bemSurf.np, bemSurf.ntri);

    return true;
}

//=============================================================================================================

bool Surf2Bem::shiftVertices(MNEBemSurface& surf, float shift)
{
    if (shift == 0.0f)
        return true;

    //
    // Shift vertices outward along normals
    //
    for (int k = 0; k < surf.np; ++k) {
        surf.rr(k, 0) += shift * surf.nn(k, 0);
        surf.rr(k, 1) += shift * surf.nn(k, 1);
        surf.rr(k, 2) += shift * surf.nn(k, 2);
    }

    // Recompute normals after shifting
    surf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(surf.rr), Eigen::MatrixX3i(surf.itris));

    Log::info("{} vertex locations shifted by {:6.1f} mm.", SurfaceChecks::getNameOf(surf.id), 1000.0f * shift);

    return true;
}

//=============================================================================================================

void Surf2Bem::orderSurfaces(std::vector<MNEBemSurface>& surfs)
{
    //
    // Reorder to head, skull, brain for proper BEM nesting
    // Only reorder if all three types are present
    //
    if (surfs.size() != 3)
        return;

    int headIdx = -1, skullIdx = -1, brainIdx = -1;
    for (int k = 0; k < 3; ++k) {
        if (surfs[k].id == FIFFV_BEM_SURF_ID_HEAD)
            headIdx = k;
        else if (surfs[k].id == FIFFV_BEM_SURF_ID_SKULL)
            skullIdx = k;
        else if (surfs[k].id == FIFFV_BEM_SURF_ID_BRAIN)
            brainIdx = k;
    }

    if (headIdx >= 0 && skullIdx >= 0 && brainIdx >= 0) {
        std::vector<MNEBemSurface> ordered(3);
        ordered[0] = surfs[static_cast<std::size_t>(headIdx)];
        ordered[1] = surfs[static_cast<std::size_t>(skullIdx)];
        ordered[2] = surfs[static_cast<std::size_t>(brainIdx)];
        surfs = ordered;
    }
}
