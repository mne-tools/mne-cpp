//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2001
 * @brief    List source space information.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_spaces.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_types.h>
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
/**
 * Qt's asprintf, QString::number and QTextStream round exact decimal ties away from zero where std::format rounds
 * them to even. Moves a tie at @p decimals to the neighbouring double away from zero, so that std::format writes
 * the digits Qt wrote.
 */
static double listSrcQtTie(double value, int decimals)
{
    const double scaled = std::ldexp(value, decimals + 1);
    // A tie at decimals is an odd multiple of 2^-(decimals + 1)
    if (std::isfinite(scaled) && scaled == std::trunc(scaled) && std::fmod(scaled, 2.0) != 0.0) {
        return std::nextafter(value, value < 0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());
    }
    return value;
}

//=============================================================================================================

static const char* coordFrameName(int frame)
{
    switch (frame) {
        case FIFFV_COORD_MRI:
            return "MRI (surface RAS)";
        case FIFFV_COORD_HEAD:
            return "Head";
        case FIFFV_COORD_DEVICE:
            return "Device";
        case FIFFV_COORD_UNKNOWN:
            return "Unknown";
        default:
            return "Other";
    }
}

//=============================================================================================================

static const char* spaceTypeName(int type)
{
    switch (type) {
        case FIFFV_MNE_SPACE_SURFACE:
            return "Surface";
        case FIFFV_MNE_SPACE_VOLUME:
            return "Volume";
        case FIFFV_MNE_SPACE_DISCRETE:
            return "Discrete";
        default:
            return "Unknown";
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("List source space information in various output formats.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"src"}, "Source space FIFF file.", "file", {}});
    parser.add_option({{"pnt"}, "Output point file (MRIlab format).", "file", {}});
    parser.add_option({{"dip"}, "Output dipole file (MRIlab format).", "file", {}});
    parser.add_option({{"vert"}, "Output vertex file (text).", "file", {}});
    parser.add_option({{"all"}, "List all vertices (not just active).", {}, {}});
    parser.add_option({{"coord"}, "Coordinate frame: head or mri.", "name", "mri"});
    parser.process(argc, argv, "mne_list_source_space", PROGRAM_VERSION);

    const std::string srcName = parser.value("src");
    const std::string pntName = parser.value("pnt");
    const std::string dipName = parser.value("dip");
    const std::string vertName = parser.value("vert");
    bool listAll = parser.is_set("all");
    [[maybe_unused]] int coordFrame = FIFFV_COORD_MRI;
    const std::string coordStr = parser.value("coord");
    if (coordStr == "head")
        coordFrame = FIFFV_COORD_HEAD;
    else if (coordStr == "mri")
        coordFrame = FIFFV_COORD_MRI;
    else {
        Log::critical("Unknown coordinate frame: {} (use head or mri)", coordStr);
        return 1;
    }

    if (srcName.empty()) {
        Log::critical("--src is required.");
        return 1;
    }

    // Read source spaces
    auto read = MNESourceSpaces::read(path_from_utf8(srcName));
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const MNESourceSpaces& sourceSpaces = *read;

    if (sourceSpaces.size() == 0) {
        Log::critical("No source spaces found in file.");
        return 1;
    }

    // Print summary
    Log::info("Source space file : {}", srcName);
    Log::info("Number of spaces  : {}", sourceSpaces.size());
    Log::info("{}", "");

    int totalActive = 0;
    int totalVerts = 0;

    for (int s = 0; s < sourceSpaces.size(); s++) {
        const MNESourceSpace& space = sourceSpaces[s];

        int nvert = space.np;
        int nuse = space.nuse;
        int ntri = space.ntri;
        totalActive += nuse;
        totalVerts += nvert;

        Log::info("--- Space {} ---", s + 1);
        Log::info("  Type            : {}", spaceTypeName(space.type));
        Log::info("  Vertices        : {}", nvert);
        Log::info("  Active vertices : {}", nuse);
        Log::info("  Triangles       : {}", ntri);
        Log::info("  Coord frame     : {}", coordFrameName(space.coord_frame));

        if (space.id == FIFFV_MNE_SURF_LEFT_HEMI)
            Log::info("  Hemisphere      : Left");
        else if (space.id == FIFFV_MNE_SURF_RIGHT_HEMI)
            Log::info("  Hemisphere      : Right");

        if (nuse > 0 && space.rr.rows() > 0) {
            // Compute bounding box of active vertices
            float xmin = 1e10, xmax = -1e10;
            float ymin = 1e10, ymax = -1e10;
            float zmin = 1e10, zmax = -1e10;

            for (int v = 0; v < nvert; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                float x = space.rr(v, 0) * 1000.0f;
                float y = space.rr(v, 1) * 1000.0f;
                float z = space.rr(v, 2) * 1000.0f;
                if (x < xmin)
                    xmin = x;
                if (x > xmax)
                    xmax = x;
                if (y < ymin)
                    ymin = y;
                if (y > ymax)
                    ymax = y;
                if (z < zmin)
                    zmin = z;
                if (z > zmax)
                    zmax = z;
            }
            Log::info("  Extent (mm)     : x [{:.1f}, {:.1f}] y [{:.1f}, {:.1f}] z [{:.1f}, {:.1f}]", xmin, xmax, ymin, ymax, zmin, zmax);
        }
        Log::info("{}", "");
    }

    Log::info("Total active vertices : {} / {}", totalActive, totalVerts);

    // Write point file (MRIlab format)
    if (!pntName.empty()) {
        std::ofstream pnt(path_from_utf8(pntName));
        if (!pnt) {
            Log::critical("Cannot open output file: {}", pntName);
            return 1;
        }

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                pnt << std::format("{:10.4f} {:10.4f} {:10.4f}\n",
                                   listSrcQtTie(space.rr(v, 0) * 1000.0f, 4),
                                   listSrcQtTie(space.rr(v, 1) * 1000.0f, 4),
                                   listSrcQtTie(space.rr(v, 2) * 1000.0f, 4));
            }
        }
        Log::info("Wrote point file: {}", pntName);
    }

    // Write dipole file (MRIlab format: position + orientation)
    if (!dipName.empty()) {
        std::ofstream dip(path_from_utf8(dipName));
        if (!dip) {
            Log::critical("Cannot open output file: {}", dipName);
            return 1;
        }

        int dipNum = 1;
        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                dip << std::format("{:5} {:8.3f} {:8.3f} {:8.3f}  {:7.4f} {:7.4f} {:7.4f}\n",
                                   dipNum++,
                                   listSrcQtTie(space.rr(v, 0) * 1000.0f, 3), listSrcQtTie(space.rr(v, 1) * 1000.0f, 3), listSrcQtTie(space.rr(v, 2) * 1000.0f, 3),
                                   listSrcQtTie(space.nn(v, 0), 4), listSrcQtTie(space.nn(v, 1), 4), listSrcQtTie(space.nn(v, 2), 4));
            }
        }
        Log::info("Wrote dipole file: {}", dipName);
    }

    // Write vertex file (text: vertex_number x y z)
    if (!vertName.empty()) {
        std::ofstream vert(path_from_utf8(vertName));
        if (!vert) {
            Log::critical("Cannot open output file: {}", vertName);
            return 1;
        }

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                vert << std::format("{:6} {:10.4f} {:10.4f} {:10.4f}\n",
                                    v,
                                    listSrcQtTie(space.rr(v, 0) * 1000.0f, 4), listSrcQtTie(space.rr(v, 1) * 1000.0f, 4), listSrcQtTie(space.rr(v, 2) * 1000.0f, 4));
            }
        }
        Log::info("Wrote vertex file: {}", vertName);
    }

    return 0;
}
