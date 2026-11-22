//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    CLI tool to create SSP projectors that suppress activity from cortical ROIs.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <mne/mne_source_space.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_types.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_label_ssp");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //
    // CLI argument parsing
    //
    QCommandLineParser parser;
    parser.setApplicationDescription("Create SSP projectors from cortical label ROIs");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fwdOpt("fwd", "Forward solution file.", "file");
    QCommandLineOption labelOpt("label", "Label file (can be specified multiple times).", "file");
    QCommandLineOption ncompOpt("ncomp", "Number of components per label (default: 1).", "n", "1");
    QCommandLineOption outOpt("out", "Output projector file.", "file");

    parser.addOption(fwdOpt);
    parser.addOption(labelOpt);
    parser.addOption(ncompOpt);
    parser.addOption(outOpt);

    parser.process(app);

    //
    // Validate required arguments
    //
    if (!parser.isSet(fwdOpt)) {
        Log::critical("Error: --fwd is required.");
        return 1;
    }
    if (!parser.isSet(labelOpt)) {
        Log::critical("Error: at least one --label is required.");
        return 1;
    }
    if (!parser.isSet(outOpt)) {
        Log::critical("Error: --out is required.");
        return 1;
    }

    const QString fwdFile = parser.value(fwdOpt);
    const QStringList labelFiles = parser.values(labelOpt);
    const int ncomp = parser.value(ncompOpt).toInt();
    const QString outFile = parser.value(outOpt);

    if (ncomp < 1) {
        Log::critical("Error: --ncomp must be >= 1.");
        return 1;
    }

    //
    // Load forward solution
    //
    Log::info("Reading forward solution from {}...", fwdFile.toStdString());
    auto fwdRead = MNEForwardSolution::read(fwdFile.toStdU16String());
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

    for (const QString& labelFile : labelFiles) {
        Log::info("Processing label: {}", labelFile.toStdString());

        //
        // Read label vertices
        //
        const std::optional<VectorXi> read = MNESourceSpace::read_label(labelFile.toStdU16String());
        if (!read) {
            Log::critical("Failed to read label file {}.", labelFile.toStdString());
            return 1;
        }
        const VectorXi& labelVerts = *read;

        if (labelVerts.size() == 0) {
            Log::critical("Label {} contains no vertices.", labelFile.toStdString());
            return 1;
        }

        Log::info("  Label has {} vertices", static_cast<ptrdiff_t>(labelVerts.size()));

        //
        // Build set of source indices present in the forward solution
        //
        // source_rr has nsource rows; we need to match label vertex numbers
        // to the vertno arrays in the source spaces
        //
        QVector<int> colIndices;
        int srcOffset = 0;
        for (int s = 0; s < fwd.src.size(); ++s) {
            const VectorXi& vertno = fwd.src[s].vertno;
            for (int j = 0; j < vertno.size(); ++j) {
                for (int lv = 0; lv < labelVerts.size(); ++lv) {
                    if (vertno[j] == labelVerts[lv]) {
                        for (int c = 0; c < colsPerSource; ++c)
                            colIndices.append((srcOffset + j) * colsPerSource + c);
                        break;
                    }
                }
            }
            srcOffset += vertno.size();
        }

        if (colIndices.isEmpty()) {
            Log::info("  Warning: no forward solution vertices matched label — skipping.");
            continue;
        }

        Log::info("  Matched {} gain-matrix columns", static_cast<long long>(colIndices.size()));

        //
        // Extract restricted gain matrix columns
        //
        MatrixXd Gsub(G.rows(), colIndices.size());
        for (int c = 0; c < colIndices.size(); ++c)
            Gsub.col(c) = G.col(colIndices[c]);

        //
        // Compute SVD of the restricted gain matrix
        //
        JacobiSVD<MatrixXd> svd(Gsub, ComputeThinU);
        const MatrixXd& U = svd.matrixU();

        int nUse = qMin(ncomp, static_cast<int>(U.cols()));

        //
        // Each left-singular vector becomes a projector
        //
        for (int k = 0; k < nUse; ++k) {
            // Build named matrix: 1 row x nchan cols
            MatrixXd projData(1, fwd.nchan);
            projData.row(0) = U.col(k).transpose();

            const StringList rowNames{QString("Label-SSP-%1-comp-%2")
                                          .arg(QFileInfo(labelFile).baseName())
                                          .arg(k + 1)
                                          .toStdString()};

            FiffNamedMatrix namedMat(1, fwd.nchan, rowNames, chNames, projData);
            FiffProj proj(FIFFV_PROJ_ITEM_FIELD, false,
                          QString("Label-SSP %1 #%2").arg(QFileInfo(labelFile).baseName()).arg(k + 1).toStdString(),
                          namedMat);

            projectors.push_back(proj);
        }

        Log::info("  Created {} projector(s) from {}", nUse, labelFile.toStdString());
    }

    if (projectors.empty()) {
        Log::critical("No projectors were created.");
        return 1;
    }

    //
    // Write projectors to output file
    //
    Log::info("Writing {} projector(s) to {}...", static_cast<long long>(projectors.size()), outFile.toStdString());

    auto stream = FiffStream::start_file(outFile.toStdU16String());

    if (!stream) {
        Log::critical("Could not open {} for writing.", outFile.toStdString());
        return 1;
    }

    stream->write_proj(projectors);
    stream->end_file();

    Log::info("Finished.");
    return 0;
}
