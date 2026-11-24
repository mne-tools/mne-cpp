//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2001
 * @brief    Export BEM surface data to ASCII text or FreeSurfer format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>

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

#include <cmath>
#include <format>
#include <fstream>
#include <limits>
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
    CommandLineParser parser("Produce ASCII data from a BEM FIFF file.\n\nLists vertices and triangles of specified BEM surfaces.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"bem"}, "BEM model file.", "name", {}});
    parser.add_option({{"out"}, "Output text file.", "name", {}});
    parser.add_option({{"id"}, "Surface id to list.", "id", "4"});
    parser.add_option({{"gdipoli"}, "Oostendorp format (vertices + triangles).", {}, {}});
    parser.add_option({{"xfit"}, "xfit format (same as --gdipoli --meters).", {}, {}});
    parser.add_option({{"old"}, "Old stream format.", {}, {}});
    parser.add_option({{"meters"}, "Output in meters (default: millimeters).", {}, {}});
    parser.process(argc, argv, "mne_list_bem", PROGRAM_VERSION);

    const std::string bemName = parser.value("bem");
    const std::string outName = parser.value("out");
    int surfId = to_int(parser.value("id")).value_or(0);
    bool thomOutput = false;
    bool oldOutput = false;
    float lengthMult = 1000.0f;

    if (parser.is_set("gdipoli")) {
        thomOutput = true;
        oldOutput = false;
    }
    if (parser.is_set("xfit")) {
        thomOutput = true;
        oldOutput = false;
        lengthMult = 1.0f;
    }
    if (parser.is_set("old")) {
        thomOutput = false;
        oldOutput = true;
    }
    if (parser.is_set("meters")) {
        lengthMult = 1.0f;
    }

    if (bemName.empty() || outName.empty()) {
        Log::critical("Both --bem and --out are required.");
        std::print("{}", parser.help_text("mne_list_bem"));
        return 1;
    }

    Log::info("input  file : {}", bemName);
    Log::info("output file : {}", outName);
    Log::info("surface id  : {}", surfId);
    if (lengthMult == 1.0f)
        Log::info("output in meters.");
    else
        Log::info("output in millimeters.");
    if (thomOutput)
        Log::info("output in Oostendorp (xfit) format.");
    else if (oldOutput)
        Log::info("output in old stream format.");
    else
        Log::info("output the vertex coordinates only.");

    // Read BEM surfaces
    auto read = MNEBem::read(path_from_utf8(bemName));
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const MNEBem& bem = *read;

    // Find the requested surface
    int surfIdx = -1;
    for (int i = 0; i < std::ssize(bem); i++) {
        if (bem[i].id == surfId) {
            surfIdx = i;
            break;
        }
    }
    if (surfIdx < 0) {
        Log::critical("Surface id {} not found in {}", surfId, bemName);
        return 1;
    }

    const MNEBemSurface& surf = bem[surfIdx];

    // Write output
    std::ofstream out(path_from_utf8(outName));
    if (!out) {
        Log::critical("Cannot open output file: {}", outName);
        return 1;
    }

    if (oldOutput) {
        out << std::format("{:5}\n", surf.np);
        for (int k = 0; k < surf.np; k++)
            out << std::format("{:5} {:9.4f} {:9.4f} {:9.4f}\n",
                               k + 1,
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 0), 4),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 1), 4),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 2), 4));
        out << std::format("{:5}\n", surf.ntri);
        for (int k = 0; k < surf.ntri; k++) {
            out << std::format("{:5} {:4} {:4} {:4}\n",
                               k + 1,
                               surf.itris(k, 0) + 1,
                               surf.itris(k, 2) + 1,
                               surf.itris(k, 1) + 1);
        }
    } else if (thomOutput) {
        out << std::format("{:5}\n", surf.np);
        for (int k = 0; k < surf.np; k++)
            out << std::format("{:5} {:9.4f} {:9.4f} {:9.4f}\n",
                               k + 1,
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 0), 4),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 1), 4),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 2), 4));
        out << std::format("{:5}\n", surf.ntri);
        for (int k = 0; k < surf.ntri; k++) {
            out << std::format("{:5} {:4} {:4} {:4}\n",
                               k + 1,
                               surf.itris(k, 0) + 1,
                               surf.itris(k, 1) + 1,
                               surf.itris(k, 2) + 1);
        }
    } else {
        for (int k = 0; k < surf.np; k++)
            out << std::format("{:10.3f} {:10.3f} {:10.3f}\n",
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 0), 3),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 1), 3),
                               mne::tie_away_from_zero(lengthMult * surf.rr(k, 2), 3));
    }

    out.close();
    Log::info("done.");

    return 0;
}
