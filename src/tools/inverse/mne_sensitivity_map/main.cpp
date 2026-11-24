//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2004
 * @brief    Compute sensitivity maps from a forward solution using SVD of the gain matrix.
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
#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
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
    CommandLineParser parser("Compute sensitivity maps from forward solution gain matrix.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"fwd"}, "Forward solution FIFF file.", "file", {}});
    parser.add_option({{"out"}, "Output sensitivity map (text format).", "file", {}});
    parser.add_option({{"method"}, "Method: 'norm' (column norms) or 'svd' (leading singular value).", "name", "norm"});

    parser.process(argc, argv, "mne_sensitivity_map", PROGRAM_VERSION);

    const std::string fwdFile = parser.value("fwd");
    const std::string outFile = parser.value("out");
    const std::string method = to_lower(parser.value("method"));

    if (fwdFile.empty()) {
        Log::critical("--fwd is required.");
        std::print("{}", parser.help_text("mne_sensitivity_map"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_sensitivity_map"));
        return 1;
    }

    // Load forward solution
    auto fwdRead = MNEForwardSolution::read(path_from_utf8(fwdFile));
    if (!fwdRead) {
        Log::critical("{}", fwdRead.error().what());
        return 1;
    }
    MNEForwardSolution& fwd = *fwdRead;

    MatrixXd G = fwd.sol.data;
    const int nChan = static_cast<int>(G.rows());
    const int nSrc = static_cast<int>(G.cols());
    bool isFixed = fwd.isFixedOrient();

    Log::info("Forward: {} channels x {} sources ({} orientation)", nChan, nSrc, isFixed ? "fixed" : "free");

    // Compute sensitivity per source
    int nSourcePoints = isFixed ? nSrc : nSrc / 3;
    VectorXd sensitivity(nSourcePoints);

    if (method == "norm") {
        // Column norm method: for each source, compute norm of gain columns
        for (int s = 0; s < nSourcePoints; ++s) {
            if (isFixed) {
                sensitivity(s) = G.col(s).norm();
            } else {
                // Free orientation: take 3 columns per source
                MatrixXd Gs = G.block(0, s * 3, nChan, 3);
                sensitivity(s) = Gs.norm(); // Frobenius norm
            }
        }
    } else if (method == "svd") {
        // SVD method: leading singular value per source
        for (int s = 0; s < nSourcePoints; ++s) {
            MatrixXd Gs;
            if (isFixed) {
                Gs = G.col(s);
            } else {
                Gs = G.block(0, s * 3, nChan, 3);
            }
            JacobiSVD<MatrixXd> svd(Gs);
            sensitivity(s) = svd.singularValues()(0);
        }
    } else {
        Log::critical("Unknown method: {} (use 'norm' or 'svd')", method);
        return 1;
    }

    // Normalize to [0, 1]
    double maxSens = sensitivity.maxCoeff();
    if (maxSens > 0)
        sensitivity /= maxSens;

    Log::info("Sensitivity range: {:g} .. {:g} (normalized)", sensitivity.minCoeff(), sensitivity.maxCoeff());

    // Write output
    std::ofstream out(path_from_utf8(outFile));
    if (!out) {
        Log::critical("Cannot write: {}", outFile);
        return 1;
    }
    out << "# Sensitivity map (" << method << " method)\n";
    out << "# " << nSourcePoints << " sources\n";
    for (int s = 0; s < nSourcePoints; ++s)
        out << std::format("{} {:.10g}\n", s, sensitivity(s));
    out.close();

    Log::info("Written sensitivity map to: {}", outFile);
    return 0;
}
