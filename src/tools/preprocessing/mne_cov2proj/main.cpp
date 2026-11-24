//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Convert noise covariance matrix eigenvectors to SSP projection operators.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
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
    CommandLineParser parser("Convert noise covariance eigenvectors to SSP projection operators.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"cov"}, "Input noise covariance FIFF file.", "file", {}});
    parser.add_option({{"raw"}, "Raw FIFF file (for channel info).", "file", {}});
    parser.add_option({{"nproj"}, "Number of projectors to create.", "n", "5"});
    parser.add_option({{"out"}, "Output projection FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_cov2proj", PROGRAM_VERSION);

    const std::string covFile = parser.value("cov");
    const std::string rawFile = parser.value("raw");
    const std::string outFile = parser.value("out");
    int nProj = to_int(parser.value("nproj")).value_or(0);

    if (covFile.empty()) {
        Log::critical("--cov is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Read covariance
    FiffCov cov;
    try {
        cov = FiffCov(path_from_utf8(covFile));
    } catch (const std::exception& error) {
        Log::critical("Cannot read covariance from {}: {}", covFile, error.what());
        return 1;
    }
    if (cov.empty()) {
        Log::critical("Cannot read covariance from: {}", covFile);
        return 1;
    }
    Log::info("Read covariance: {} x {} channels", cov.dim, cov.dim);

    // Get channel names
    StringList chNames = cov.names;
    if (chNames.empty() && !rawFile.empty()) {
        FiffRawData raw(path_from_utf8(rawFile));
        chNames = raw.info.ch_names;
    }

    // Eigendecomposition of the covariance matrix
    MatrixXd covMat = cov.data;
    SelfAdjointEigenSolver<MatrixXd> eig(covMat);
    if (eig.info() != Success) {
        Log::critical("Eigendecomposition failed.");
        return 1;
    }

    VectorXd eigenvalues = eig.eigenvalues();
    MatrixXd eigenvectors = eig.eigenvectors();

    Log::info("Eigenvalue range: {:g} .. {:g}", eigenvalues(0), eigenvalues(eigenvalues.size() - 1));

    // Select the top nProj eigenvectors (largest eigenvalues = most noise)
    int nEig = static_cast<int>(eigenvalues.size());
    if (nProj > nEig)
        nProj = nEig;

    // Eigenvectors are in ascending order; take from the end
    std::vector<FiffProj> projs;
    for (int p = 0; p < nProj; ++p) {
        int idx = nEig - 1 - p;
        FiffProj proj;
        proj.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF; // Generic projection
        proj.active = false;
        proj.desc = std::format("Cov eigenvector #{} (eigenvalue={:.6g})", p + 1, eigenvalues(idx));

        // Build the named matrix for this projector
        proj.data.nrow = 1;
        proj.data.ncol = nEig;
        proj.data.col_names = chNames;
        proj.data.data = eigenvectors.col(idx).transpose();

        projs.push_back(proj);

        Log::info("  Projector {}: eigenvalue = {:g}", p + 1, eigenvalues(idx));
    }

    // Write projectors to file
    if (!std::ofstream(path_from_utf8(outFile), std::ios::binary)) {
        Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }
    auto stream = FiffStream::start_file(path_from_utf8(outFile));
    if (!stream) {
        Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }

    stream->start_block(FIFFB_MNE);
    stream->write_proj(projs);
    stream->end_block(FIFFB_MNE);
    stream->end_file();

    Log::info("Written {} projectors to: {}", nProj, outFile);
    return 0;
}
