//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Create a uniform STC where every vertex has the same value.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_source_spaces.h>
#include <mne/fiff/fiff_stream.h>
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

#include <optional>
#include <print>
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Create a uniform STC where every vertex has the same value.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"src"}, "Source space FIFF file.", "file", {}});
    parser.add_option({{"val"}, "Uniform value for all vertices (default: 1.0).", "value", "1.0"});
    parser.add_option({{"out"}, "Output STC file path.", "file", {}});

    parser.process(argc, argv, "mne_make_uniform_stc", PROGRAM_VERSION);

    if (!parser.is_set("src") || !parser.is_set("out")) {
        Log::critical("Both --src and --out are required.");
        std::print("{}", parser.help_text("mne_make_uniform_stc"));
        return 1;
    }

    const std::optional<double> val = to_double(parser.value("val"));
    if (!val) {
        Log::critical("Invalid --val value: {}", parser.value("val"));
        return 1;
    }
    const double uniformVal = *val;

    //--- Read source space ---
    auto read = MNESourceSpaces::read(path_from_utf8(parser.value("src")));
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const MNESourceSpaces& srcSpaces = *read;

    //--- Collect in-use vertices from all hemispheres ---
    std::vector<VectorXi> vertnoList = srcSpaces.get_vertno();
    int totalVerts = 0;
    for (const auto& v : vertnoList)
        totalVerts += static_cast<int>(v.size());

    if (totalVerts == 0) {
        Log::critical("No in-use vertices found in source space.");
        return 1;
    }

    // Concatenate vertex numbers
    VectorXi allVertices(totalVerts);
    int offset = 0;
    for (const auto& v : vertnoList) {
        allVertices.segment(offset, v.size()) = v;
        offset += static_cast<int>(v.size());
    }

    //--- Build uniform source estimate (single time point) ---
    MatrixXd data = MatrixXd::Constant(totalVerts, 1, uniformVal);
    float tmin = 0.0f;
    float tstep = 1.0f;

    InvSourceEstimate stc(data, allVertices, tmin, tstep);

    //--- Write output ---
    if (!stc.write(path_from_utf8(parser.value("out")))) {
        Log::critical("Failed to write output STC.");
        return 1;
    }

    Log::info("Created uniform STC with {} vertices, value = {} -> {}", totalVerts, uniformVal, parser.value("out"));
    return 0;
}
