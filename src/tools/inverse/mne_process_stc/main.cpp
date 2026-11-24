//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Process STC files: scale, export to ASCII, copy.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/inv/inv_source_estimate.h>
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
#include <optional>
#include <print>

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
    CommandLineParser parser("Process STC files: scale, export to ASCII, copy.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"in"}, "Input STC file.", "file", {}});
    parser.add_option({{"out"}, "Output binary STC file.", "file", {}});
    parser.add_option({{"outasc"}, "Output ASCII file.", "file", {}});
    parser.add_option({{"scaleby"}, "Multiply values by this factor.", "factor", {}});
    parser.add_option({{"scaleto"}, "Scale data so that the maximum equals this value.", "value", {}});

    parser.process(argc, argv, "mne_process_stc", PROGRAM_VERSION);

    if (!parser.is_set("in")) {
        Log::critical("--in is required.");
        std::print("{}", parser.help_text("mne_process_stc"));
        return 1;
    }

    if (!parser.is_set("out") && !parser.is_set("outasc")) {
        Log::critical("At least one of --out or --outasc is required.");
        std::print("{}", parser.help_text("mne_process_stc"));
        return 1;
    }

    //--- Read input STC ---
    auto read = InvSourceEstimate::read(path_from_utf8(parser.value("in")));
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    InvSourceEstimate& stc = *read;
    //--- Apply scaling ---
    if (parser.is_set("scaleby")) {
        const std::optional<double> factor = to_double(parser.value("scaleby"));
        if (!factor) {
            Log::critical("Invalid --scaleby value: {}", parser.value("scaleby"));
            return 1;
        }
        stc.data *= *factor;
        Log::info("Scaled data by factor {}", *factor);
    }

    if (parser.is_set("scaleto")) {
        const std::optional<double> target = to_double(parser.value("scaleto"));
        if (!target) {
            Log::critical("Invalid --scaleto value: {}", parser.value("scaleto"));
            return 1;
        }
        double maxVal = stc.data.array().abs().maxCoeff();
        if (std::abs(maxVal) <= 1e-12) {
            Log::warning("Data maximum is zero — cannot scale to target.");
        } else {
            stc.data *= (*target / maxVal);
            Log::info("Scaled data so max = {}", *target);
        }
    }

    //--- Write binary STC ---
    if (parser.is_set("out")) {
        if (!stc.write(path_from_utf8(parser.value("out")))) {
            Log::critical("Failed to write output STC.");
            return 1;
        }
        Log::info("Wrote binary STC: {}", parser.value("out"));
    }

    //--- Write ASCII ---
    if (parser.is_set("outasc")) {
        std::ofstream ascFile(path_from_utf8(parser.value("outasc")));
        if (!ascFile) {
            Log::critical("Cannot open ASCII output file: {}", parser.value("outasc"));
            return 1;
        }
        for (Index v = 0; v < stc.data.rows(); ++v) {
            for (Index t = 0; t < stc.data.cols(); ++t) {
                ascFile << std::format("{} {} {:g}\n", stc.vertices(v), t, stc.data(v, t));
            }
        }
        ascFile.close();
        Log::info("Wrote ASCII file: {}", parser.value("outasc"));
    }

    return 0;
}
