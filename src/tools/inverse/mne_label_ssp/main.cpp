//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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
        qCritical("Error: --fwd is required.");
        return 1;
    }
    if (!parser.isSet(labelOpt)) {
        qCritical("Error: at least one --label is required.");
        return 1;
    }
    if (!parser.isSet(outOpt)) {
        qCritical("Error: --out is required.");
        return 1;
    }

    const QString fwdFile = parser.value(fwdOpt);
    const QStringList labelFiles = parser.values(labelOpt);
    const int ncomp = parser.value(ncompOpt).toInt();
    const QString outFile = parser.value(outOpt);

    if (ncomp < 1) {
        qCritical("Error: --ncomp must be >= 1.");
        return 1;
    }

    //
    // Load forward solution
    //
    fprintf(stderr, "Reading forward solution from %s...\n", fwdFile.toUtf8().constData());
    QFile fwdIO(fwdFile);
    MNEForwardSolution fwd(fwdIO);

    if (fwd.isEmpty()) {
        qCritical("Failed to read forward solution from %s.", fwdFile.toUtf8().constData());
        return 1;
    }

    fprintf(stderr, "Forward solution: %d channels x %d sources\n", fwd.nchan, fwd.nsource);

    //
    // Get the gain matrix and channel names
    //
    const MatrixXd& G = fwd.sol->data;       // nchan x nsource (or nsource*3 for free ori)
    const QStringList& chNames = fwd.sol->row_names;

    //
    // Determine source columns per vertex depending on orientation type
    //
    const int colsPerSource = (fwd.isFixedOrient()) ? 1 : 3;

    //
    // Process each label
    //
    QList<FiffProj> projectors;

    for (const QString& labelFile : labelFiles) {
        fprintf(stderr, "Processing label: %s\n", labelFile.toUtf8().constData());

        //
        // Read label vertices
        //
        VectorXi labelVerts;
        if (MNESourceSpace::read_label(labelFile, labelVerts) != 0) {
            qCritical("Failed to read label file %s.", labelFile.toUtf8().constData());
            return 1;
        }

        if (labelVerts.size() == 0) {
            qCritical("Label %s contains no vertices.", labelFile.toUtf8().constData());
            return 1;
        }

        fprintf(stderr, "  Label has %td vertices\n", static_cast<ptrdiff_t>(labelVerts.size()));

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
            fprintf(stderr, "  Warning: no forward solution vertices matched label — skipping.\n");
            continue;
        }

        fprintf(stderr, "  Matched %lld gain-matrix columns\n", static_cast<long long>(colIndices.size()));

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

            QStringList rowNames;
            rowNames << QString("Label-SSP-%1-comp-%2")
                        .arg(QFileInfo(labelFile).baseName())
                        .arg(k + 1);

            FiffNamedMatrix namedMat(1, fwd.nchan, rowNames, chNames, projData);
            FiffProj proj(FIFFV_PROJ_ITEM_FIELD, false,
                          QString("Label-SSP %1 #%2").arg(QFileInfo(labelFile).baseName()).arg(k + 1),
                          namedMat);

            projectors.append(proj);
        }

        fprintf(stderr, "  Created %d projector(s) from %s\n", nUse, labelFile.toUtf8().constData());
    }

    if (projectors.isEmpty()) {
        qCritical("No projectors were created.");
        return 1;
    }

    //
    // Write projectors to output file
    //
    fprintf(stderr, "Writing %lld projector(s) to %s...\n", static_cast<long long>(projectors.size()), outFile.toUtf8().constData());

    QFile outIO(outFile);
    FiffStream::SPtr stream = FiffStream::start_file(outIO);

    if (!stream) {
        qCritical("Could not open %s for writing.", outFile.toUtf8().constData());
        return 1;
    }

    stream->write_proj(projectors);
    stream->end_file();

    fprintf(stderr, "Finished.\n");
    return 0;
}
