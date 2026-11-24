//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Average multiple forward solutions (weighted by number of averages).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_forward_solution.h>
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

#include <fstream>
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Average multiple forward solutions.\n\nUse --fwd multiple times or --listfile to specify input files.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"fwd"}, "Forward solution FIFF file (repeat for each).", "file", ""});
    parser.add_option({{"weights"}, "Comma-separated weights (default: equal).", "w1,w2,...", ""});
    parser.add_option({{"listfile"}, "Text file with one fwd filename per line.", "file", ""});
    parser.add_option({{"out"}, "Output averaged forward solution FIFF file.", "file", ""});

    parser.process(argc, argv, "mne_average_forward_solutions", PROGRAM_VERSION);

    StringList fwdFiles = parser.is_set("fwd") ? parser.values("fwd") : StringList{};
    const std::string weightStr = parser.value("weights");
    const std::string listFile = parser.value("listfile");
    const std::string outFile = parser.value("out");

    // Read files from listfile if specified
    if (!listFile.empty()) {
        std::ifstream lf(path_from_utf8(listFile));
        if (!lf) {
            Log::critical("Cannot open list file: {}", listFile);
            return 1;
        }
        std::string rawLine;
        while (std::getline(lf, rawLine)) {
            const std::string_view line = trimmed(rawLine);
            if (!line.empty() && !line.starts_with('#'))
                fwdFiles.emplace_back(line);
        }
    }

    if (fwdFiles.size() < 2) {
        Log::critical("At least 2 forward solutions are required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Parse weights
    std::vector<double> weights;
    if (!weightStr.empty()) {
        for (const std::string& p : split(weightStr, ','))
            weights.push_back(to_double(p).value_or(0.0));
        if (weights.size() != fwdFiles.size()) {
            Log::critical("Number of weights ({}) doesn't match number of files ({})", static_cast<long long>(weights.size()), static_cast<long long>(fwdFiles.size()));
            return 1;
        }
    } else {
        // Equal weights
        weights.assign(fwdFiles.size(), 1.0);
    }

    // Normalize weights
    double wSum = 0;
    for (double w : weights)
        wSum += w;
    for (double& w : weights)
        w /= wSum;

    // Load first forward solution as template
    Log::info("Loading {} forward solutions...", static_cast<long long>(fwdFiles.size()));

    auto fwd0Read = MNEForwardSolution::read(path_from_utf8(fwdFiles[0]));
    if (!fwd0Read) {
        Log::critical("{}", fwd0Read.error().what());
        return 1;
    }
    MNEForwardSolution& fwd0 = *fwd0Read;
    Log::info("  [1] {}: {} channels x {} sources (weight={:.4f})", fwdFiles[0], static_cast<int>(fwd0.sol.data.rows()), static_cast<int>(fwd0.sol.data.cols()), weights[0]);

    const auto nChan = static_cast<int>(fwd0.sol.data.rows());
    const auto nSrc = static_cast<int>(fwd0.sol.data.cols());

    // Weighted sum
    MatrixXd avgSol = weights[0] * fwd0.sol.data;

    for (std::size_t i = 1; i < fwdFiles.size(); ++i) {
        auto fwdiRead = MNEForwardSolution::read(path_from_utf8(fwdFiles[i]));
        if (!fwdiRead) {
            Log::critical("{}", fwdiRead.error().what());
            return 1;
        }
        MNEForwardSolution& fwdi = *fwdiRead;

        if (fwdi.sol.data.rows() != nChan || fwdi.sol.data.cols() != nSrc) {
            Log::critical("Forward solution dimension mismatch in {}: {}x{} vs {}x{}", fwdFiles[i], static_cast<int>(fwdi.sol.data.rows()), static_cast<int>(fwdi.sol.data.cols()), nChan, nSrc);
            return 1;
        }

        Log::info("  [{}] {}: {} channels x {} sources (weight={:.4f})", i + 1, fwdFiles[i], nChan, nSrc, weights[i]);

        avgSol += weights[i] * fwdi.sol.data;
    }

    Log::info("Averaged forward solution: {} channels x {} sources", nChan, nSrc);

    // Write averaged forward solution
    fwd0.sol.data = avgSol;

    if (auto written = fwd0.write(path_from_utf8(outFile)); !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }

    Log::info("Written averaged forward solution to: {}", outFile);
    return 0;
}
