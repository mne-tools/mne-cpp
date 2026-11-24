//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2002
 * @brief    Create a volumetric source space inside a BEM surface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_space.h>
#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
    CommandLineParser parser("Create a volumetric source space inside a BEM surface.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"bem"}, "BEM FIFF file (inner skull surface used as boundary).", "file", {}});
    parser.add_option({{"grid"}, "Grid spacing in mm.", "mm", "7.0"});
    parser.add_option({{"exclude"}, "Exclude points closer to CM than this (mm).", "mm", "0"});
    parser.add_option({{"mindist"}, "Minimum distance from surface in mm.", "mm", "5.0"});
    parser.add_option({{"out"}, "Output source space FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_volume_source_space", PROGRAM_VERSION);

    const std::string bemFile = parser.value("bem");
    const std::string outFile = parser.value("out");
    float gridMm = static_cast<float>(to_double(parser.value("grid")).value_or(0.0));
    float excludeMm = static_cast<float>(to_double(parser.value("exclude")).value_or(0.0));
    float mindistMm = static_cast<float>(to_double(parser.value("mindist")).value_or(0.0));

    if (bemFile.empty()) {
        Log::critical("--bem is required.");
        std::print("{}", parser.help_text("mne_volume_source_space"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_volume_source_space"));
        return 1;
    }

    // Convert mm to meters
    float grid = gridMm / 1000.0f;
    float exclude = excludeMm / 1000.0f;
    float mindist = mindistMm / 1000.0f;

    // Read BEM file - find innermost surface
    MNEBem bem(path_from_utf8(bemFile));
    if (bem.size() == 0) {
        Log::critical("Cannot read BEM from: {}", bemFile);
        return 1;
    }

    // Find inner skull surface (smallest = innermost)
    int innerIdx = 0;
    for (int i = 1; i < std::ssize(bem); ++i) {
        if (bem[i].id == FIFFV_BEM_SURF_ID_BRAIN) {
            innerIdx = i;
            break;
        }
    }

    const MNEBemSurface& innerSurf = bem[innerIdx];
    Log::info("Using surface: {} ({} vertices, {} triangles)", MNEBemSurface::id_name(innerSurf.id), innerSurf.np, innerSurf.ntri);

    // Create MNESurface from BEM surface for the library function
    MNESurface surf;
    surf.np = innerSurf.np;
    surf.ntri = innerSurf.ntri;
    surf.rr = innerSurf.rr;
    surf.nn = innerSurf.nn;
    surf.itris = innerSurf.itris;
    surf.id = innerSurf.id;
    surf.coord_frame = innerSurf.coord_frame;

    Log::info("Creating volume source space (grid={:.1f} mm, mindist={:.1f} mm, exclude={:.1f} mm)...", gridMm, mindistMm, excludeMm);

    // Use the existing library function
    const auto sp = MNESourceSpace::make_volume_source_space(surf, grid, exclude, mindist);
    if (!sp) {
        Log::critical("Volume source space creation failed.");
        return 1;
    }

    Log::info("Volume source space: {} total points, {} active", sp->np, sp->nuse);

    // Write output
    // Fail early, as before, if the output cannot be created
    if (!std::ofstream(path_from_utf8(outFile), std::ios::binary)) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }
    auto outStream = FiffStream::start_file(path_from_utf8(outFile));
    if (!outStream) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }
    outStream->start_block(FIFFB_MNE);
    outStream->start_block(FIFFB_MNE_SOURCE_SPACE);
    sp->writeToStream(*outStream, false);
    outStream->end_block(FIFFB_MNE_SOURCE_SPACE);
    outStream->end_block(FIFFB_MNE);
    outStream->end_file();
    Log::info("Written volume source space to: {}", outFile);
    return 0;
}
