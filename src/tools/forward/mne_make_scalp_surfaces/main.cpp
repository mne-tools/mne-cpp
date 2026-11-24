//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     2009
 * @brief    Generate decimated scalp surfaces from BEM outer skin.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_types.h>
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

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Generate decimated scalp surfaces from BEM outer skin.\n\n"
                             "Reads a BEM file, finds the outer skin surface, and creates\n"
                             "decimated versions at the specified vertex counts.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"bem"}, "BEM file with outer skin surface.", "file", ""});
    parser.add_option({{"out"}, "Output directory for decimated surfaces.", "dir", ""});
    parser.add_option({{"grades"}, "Comma-separated target vertex counts (default: 2562,10242,40962).", "counts", "2562,10242,40962"});

    parser.process(argc, argv, "mne_make_scalp_surfaces", PROGRAM_VERSION);

    const std::string bemFile = parser.value("bem");
    const std::string outDir = parser.value("out");
    const std::string gradesStr = parser.value("grades");

    if (bemFile.empty()) {
        Log::critical("--bem is required.");
        return 1;
    }
    if (outDir.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Parse grades
    std::vector<int> grades;
    for (const std::string& g : split(gradesStr, ',', true)) {
        const int val = to_int(g).value_or(0);
        if (val > 0)
            grades.push_back(val);
    }
    if (grades.empty()) {
        Log::critical("No valid vertex counts specified.");
        return 1;
    }

    const std::filesystem::path bemPath = path_from_utf8(bemFile);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(bemPath, ec) || !std::ifstream(bemPath)) {
        Log::critical("Cannot read BEM from: {}", bemFile);
        return 1;
    }

    // Read BEM
    MNEBem bem(bemPath);
    if (bem.size() == 0) {
        Log::critical("Cannot read BEM from: {}", bemFile);
        return 1;
    }

    Log::info("Read BEM with {} surface(s) from {}", bem.size(), bemFile);

    // Find outer skin surface (typically the last/outermost surface, or id == FIFFV_BEM_SURF_ID_HEAD)
    auto head = std::ranges::find(bem, FIFFV_BEM_SURF_ID_HEAD, &MNEBemSurface::id);
    if (head == bem.end()) {
        head = std::prev(bem.end());
        Log::info("No explicit outer skin surface found, using the outermost surface.");
    }
    const MNEBemSurface& skin = *head;
    Log::info("Using surface {}: {} vertices, {} triangles", head - bem.begin(), skin.np, skin.ntri);

    // Create output directory
    const std::filesystem::path outDirPath = path_from_utf8(outDir);
    std::filesystem::create_directories(outDirPath, ec);
    if (!std::filesystem::is_directory(outDirPath, ec)) {
        Log::critical("Cannot create output directory: {}", outDir);
        return 1;
    }

    // Generate decimated surfaces
    for (int targetVerts : grades) {
        Log::info("Decimating to {} vertices...", targetVerts);
        const std::vector<MNEBemSurface> surfaces =
            MNEBemSurface::makeScalpSurfaces(skin, {targetVerts});
        if (surfaces.empty()) {
            Log::critical("Could not decimate scalp surface to {} vertices.", targetVerts);
            return 1;
        }
        const MNEBemSurface& decimated = surfaces.front();

        // Write as FIFF BEM surface
        const std::filesystem::path outPath = outDirPath / std::format("scalp-{}.fif", targetVerts);

        MNEBem outBem;
        outBem.push_back(decimated);

        if (auto written = outBem.write(outPath); !written) {
            Log::critical("{}", written.error().what());
            return 1;
        }

        Log::info("  Written {} vertices, {} triangles to {}", decimated.np, decimated.ntri, utf8_from_path(outPath));
    }

    Log::info("Done.");
    return 0;
}
