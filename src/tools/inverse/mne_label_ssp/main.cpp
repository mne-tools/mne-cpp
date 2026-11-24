//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     2010
 * @brief    CLI tool to create SSP projectors that suppress activity from cortical ROIs.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_forward_solution.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_named_matrix.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// HELPERS
//=============================================================================================================

// The file name up to its first '.', like QFileInfo::baseName()
static std::string baseName(const std::string& file)
{
    const std::string name = utf8_from_path(path_from_utf8(file).filename());
    return name.substr(0, name.find('.'));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    //
    // CLI argument parsing
    //
    CommandLineParser parser("Create SSP projectors from cortical label ROIs");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"fwd"}, "Forward solution file.", "file", {}});
    parser.add_option({{"label"}, "Label file (can be specified multiple times).", "file", {}});
    parser.add_option({{"ncomp"}, "Number of components per label (default: 1).", "n", "1"});
    parser.add_option({{"out"}, "Output projector file.", "file", {}});

    parser.process(argc, argv, "mne_label_ssp", PROGRAM_VERSION);

    //
    // Validate required arguments
    //
    if (!parser.is_set("fwd")) {
        Log::critical("Error: --fwd is required.");
        return 1;
    }
    if (!parser.is_set("label")) {
        Log::critical("Error: at least one --label is required.");
        return 1;
    }
    if (!parser.is_set("out")) {
        Log::critical("Error: --out is required.");
        return 1;
    }

    const std::string fwdFile = parser.value("fwd");
    const StringList labelFiles = parser.values("label");
    const int ncomp = to_int(parser.value("ncomp")).value_or(0);
    const std::string outFile = parser.value("out");

    if (ncomp < 1) {
        Log::critical("Error: --ncomp must be >= 1.");
        return 1;
    }

    //
    // Load forward solution
    //
    Log::info("Reading forward solution from {}...", fwdFile);
    auto fwdRead = MNEForwardSolution::read(path_from_utf8(fwdFile));
    if (!fwdRead) {
        Log::critical("{}", fwdRead.error().what());
        return 1;
    }
    MNEForwardSolution& fwd = *fwdRead;

    Log::info("Forward solution: {} channels x {} sources", fwd.nchan, fwd.nsource);

    //
    // Get the gain matrix and channel names
    //
    const MatrixXd& G = fwd.sol.data; // nchan x nsource (or nsource*3 for free ori)
    const StringList& chNames = fwd.sol.row_names;

    //
    // Determine source columns per vertex depending on orientation type
    //
    const int colsPerSource = (fwd.isFixedOrient()) ? 1 : 3;

    //
    // Process each label
    //
    std::vector<FiffProj> projectors;

    for (const std::string& labelFile : labelFiles) {
        Log::info("Processing label: {}", labelFile);

        //
        // Read label vertices
        //
        const std::optional<VectorXi> read = MNESourceSpace::read_label(path_from_utf8(labelFile));
        if (!read) {
            Log::critical("Failed to read label file {}.", labelFile);
            return 1;
        }
        const VectorXi& labelVerts = *read;

        if (labelVerts.size() == 0) {
            Log::critical("Label {} contains no vertices.", labelFile);
            return 1;
        }

        Log::info("  Label has {} vertices", static_cast<ptrdiff_t>(labelVerts.size()));

        //
        // Build set of source indices present in the forward solution
        //
        // source_rr has nsource rows; we need to match label vertex numbers
        // to the vertno arrays in the source spaces
        //
        std::vector<int> colIndices;
        int srcOffset = 0;
        for (int s = 0; s < std::ssize(fwd.src); ++s) {
            const VectorXi& vertno = fwd.src[s].vertno;
            for (int j = 0; j < vertno.size(); ++j) {
                for (int lv = 0; lv < labelVerts.size(); ++lv) {
                    if (vertno[j] == labelVerts[lv]) {
                        for (int c = 0; c < colsPerSource; ++c)
                            colIndices.push_back((srcOffset + j) * colsPerSource + c);
                        break;
                    }
                }
            }
            srcOffset += static_cast<int>(vertno.size());
        }

        if (colIndices.empty()) {
            Log::info("  Warning: no forward solution vertices matched label — skipping.");
            continue;
        }

        Log::info("  Matched {} gain-matrix columns", static_cast<long long>(colIndices.size()));

        //
        // Extract restricted gain matrix columns
        //
        MatrixXd Gsub(G.rows(), std::ssize(colIndices));
        for (int c = 0; c < std::ssize(colIndices); ++c)
            Gsub.col(c) = G.col(colIndices[c]);

        //
        // Compute SVD of the restricted gain matrix
        //
        JacobiSVD<MatrixXd> svd(Gsub, ComputeThinU);
        const MatrixXd& U = svd.matrixU();

        int nUse = std::min(ncomp, static_cast<int>(U.cols()));

        //
        // Each left-singular vector becomes a projector
        //
        for (int k = 0; k < nUse; ++k) {
            // Build named matrix: 1 row x nchan cols
            MatrixXd projData(1, fwd.nchan);
            projData.row(0) = U.col(k).transpose();

            const StringList rowNames{std::format("Label-SSP-{}-comp-{}", baseName(labelFile), k + 1)};

            FiffNamedMatrix namedMat(1, fwd.nchan, rowNames, chNames, projData);
            FiffProj proj(FIFFV_PROJ_ITEM_FIELD, false,
                          std::format("Label-SSP {} #{}", baseName(labelFile), k + 1),
                          namedMat);

            projectors.push_back(proj);
        }

        Log::info("  Created {} projector(s) from {}", nUse, labelFile);
    }

    if (projectors.empty()) {
        Log::critical("No projectors were created.");
        return 1;
    }

    //
    // Write projectors to output file
    //
    Log::info("Writing {} projector(s) to {}...", static_cast<long long>(projectors.size()), outFile);

    auto stream = FiffStream::start_file(path_from_utf8(outFile));

    if (!stream) {
        Log::critical("Could not open {} for writing.", outFile);
        return 1;
    }

    stream->write_proj(projectors);
    stream->end_file();

    Log::info("Finished.");
    return 0;
}
