//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     2002
 * @brief    Transform 3D points between coordinate frames using a FIFF coordinate transform.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_constants.h>

#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/base/command_line.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <fstream>
#include <optional>
#include <print>
#include <string>
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

static int frameFromName(const std::string& name)
{
    const std::string lower = to_lower(name);
    if (lower == "head")
        return FIFFV_COORD_HEAD;
    if (lower == "mri")
        return FIFFV_COORD_MRI;
    if (lower == "device")
        return FIFFV_COORD_DEVICE;
    if (lower == "isotrak")
        return FIFFV_COORD_ISOTRAK;
    if (lower == "hpi")
        return FIFFV_COORD_HPI;
    return -1;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Transform 3D points between coordinate frames.\n\nReads a FIFF coordinate transform and applies it to a set of 3D points.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"trans"}, "FIFF coordinate transform file.", "name", ""});
    parser.add_option({{"in"}, "Input text file (one point per line: x y z).", "name", ""});
    parser.add_option({{"out"}, "Output text file.", "name", ""});
    parser.add_option({{"from"}, "Source frame name (head, mri, device).", "name", ""});
    parser.add_option({{"to"}, "Target frame name (head, mri, device).", "name", ""});

    parser.process(argc, argv, "mne_transform_points", PROGRAM_VERSION);

    const std::string transName = parser.value("trans");
    const std::string inName = parser.value("in");
    const std::string outName = parser.value("out");
    const std::string fromName = parser.value("from");
    const std::string toName = parser.value("to");

    if (transName.empty() || inName.empty() || outName.empty() || fromName.empty() || toName.empty()) {
        Log::critical("All options --trans, --in, --out, --from, and --to are required.");
        std::print("{}", parser.help_text("mne_transform_points"));
        return 1;
    }

    int fromFrame = frameFromName(fromName);
    int toFrame = frameFromName(toName);

    if (fromFrame < 0) {
        Log::critical("Unknown source frame: {}", fromName);
        return 1;
    }
    if (toFrame < 0) {
        Log::critical("Unknown target frame: {}", toName);
        return 1;
    }

    // Read the transform between the requested frames (inverted if stored the other way round)
    const auto read = FiffCoordTrans::read(path_from_utf8(transName), fromFrame, toFrame);
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const FiffCoordTrans& trans = *read;

    Log::info("Read transform: {} -> {}", std::string(FiffCoordTrans::frame_name(trans.from)), std::string(FiffCoordTrans::frame_name(trans.to)));

    // Read input points
    std::ifstream inFile(path_from_utf8(inName));
    if (!inFile) {
        Log::critical("Cannot open input file: {}", inName);
        return 1;
    }

    std::vector<Vector3f> points;
    std::string rawLine;
    while (std::getline(inFile, rawLine)) {
        const std::string_view line = trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() < 3) {
            Log::critical("Invalid line in input file: {}", line);
            return 1;
        }
        const std::optional<double> x = to_double(parts[0]);
        const std::optional<double> y = to_double(parts[1]);
        const std::optional<double> z = to_double(parts[2]);
        if (!x || !y || !z) {
            Log::critical("Cannot parse coordinates: {}", line);
            return 1;
        }
        points.emplace_back(static_cast<float>(*x), static_cast<float>(*y), static_cast<float>(*z));
    }

    Log::info("Read {} points from {}", static_cast<long long>(points.size()), inName);

    // Build matrix from points
    MatrixX3f rr(std::ssize(points), 3);
    for (Eigen::Index i = 0; i < std::ssize(points); ++i)
        rr.row(i) = points[static_cast<std::size_t>(i)].transpose();

    const MatrixX3f transformed = trans.apply_trans(rr);

    // Write output
    std::ofstream outFile(path_from_utf8(outName));
    if (!outFile) {
        Log::critical("Cannot open output file: {}", outName);
        return 1;
    }

    for (Eigen::Index i = 0; i < transformed.rows(); ++i)
        outFile << std::format("{:.8f} {:.8f} {:.8f}\n", transformed(i, 0), transformed(i, 1), transformed(i, 2));
    outFile.close();

    Log::info("Wrote {} transformed points to {}", static_cast<int>(transformed.rows()), outName);

    return 0;
}
