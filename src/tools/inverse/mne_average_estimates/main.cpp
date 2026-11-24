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
 * @brief    Compute a weighted average of STC source estimate files.
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
#include <fstream>
#include <print>
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Compute a weighted average of STC source estimate files.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"desc"}, "Description file listing <weight> <stc-path> pairs (one per line).", "file", {}});
    parser.add_option({{"out"}, "Output STC file path.", "file", {}});

    parser.process(argc, argv, "mne_average_estimates", PROGRAM_VERSION);

    if (!parser.is_set("desc") || !parser.is_set("out")) {
        Log::critical("Both --desc and --out are required.");
        std::print("{}", parser.help_text("mne_average_estimates"));
        return 1;
    }

    //--- Parse description file ---
    std::ifstream descFile(path_from_utf8(parser.value("desc")));
    if (!descFile) {
        Log::critical("Cannot open description file: {}", parser.value("desc"));
        return 1;
    }

    std::vector<std::pair<double, std::string>> entries;
    std::string rawLine;
    int lineNum = 0;
    while (std::getline(descFile, rawLine)) {
        ++lineNum;
        const std::string_view line = trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() < 2) {
            Log::critical("Invalid line {} in description file (expected: <weight> <path>).", lineNum);
            return 1;
        }

        const std::optional<double> weight = to_double(parts[0]);
        if (!weight) {
            Log::critical("Invalid weight on line {} : {}", lineNum, parts[0]);
            return 1;
        }
        entries.emplace_back(*weight, std::string(parts[1]));
    }
    descFile.close();

    if (entries.empty()) {
        Log::critical("Description file contains no entries.");
        return 1;
    }

    //--- Read STCs and compute weighted average ---
    InvSourceEstimate result;
    double weightSum = 0.0;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const double w = entries[i].first;
        const std::string& path = entries[i].second;

        auto read = InvSourceEstimate::read(path_from_utf8(path));
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        const InvSourceEstimate& stc = *read;

        if (i == 0) {
            result = stc;
            result.data = w * stc.data;
        } else {
            if (stc.data.rows() != result.data.rows() || stc.data.cols() != result.data.cols()) {
                Log::critical("STC dimension mismatch in file: {}", path);
                return 1;
            }
            result.data += w * stc.data;
        }
        weightSum += w;
    }

    if (std::abs(weightSum) <= 1e-12) {
        Log::critical("Total weight is zero — cannot normalize.");
        return 1;
    }

    result.data /= weightSum;

    //--- Write output ---
    if (!result.write(path_from_utf8(parser.value("out")))) {
        Log::critical("Failed to write output STC.");
        return 1;
    }

    Log::info("Averaged {} STC files -> {}", entries.size(), parser.value("out"));
    return 0;
}
