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

#include <fiff/fiff_cov.h>

#include <vector>
#include <fiff/fiff_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_raw_data.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

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
    mne::Log::install_qt_message_handler();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_cov2proj");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert noise covariance eigenvectors to SSP projection operators.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption covOpt("cov", "Input noise covariance FIFF file.", "file");
    parser.addOption(covOpt);

    QCommandLineOption rawOpt("raw", "Raw FIFF file (for channel info).", "file");
    parser.addOption(rawOpt);

    QCommandLineOption nprojOpt("nproj", "Number of projectors to create.", "n", "5");
    parser.addOption(nprojOpt);

    QCommandLineOption outOpt("out", "Output projection FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString covFile = parser.value(covOpt);
    QString rawFile = parser.value(rawOpt);
    QString outFile = parser.value(outOpt);
    int nProj = parser.value(nprojOpt).toInt();

    if (covFile.isEmpty()) {
        qCritical("--cov is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        qCritical("--out is required.");
        return 1;
    }

    // Read covariance
    FiffCov cov;
    try {
        cov = FiffCov(covFile.toStdU16String());
    } catch (const std::exception& error) {
        qCritical("Cannot read covariance from %s: %s",
                  qPrintable(covFile), error.what());
        return 1;
    }
    if (cov.empty()) {
        qCritical("Cannot read covariance from: %s", qPrintable(covFile));
        return 1;
    }
    qInfo("Read covariance: %d x %d channels", cov.dim, cov.dim);

    // Get channel names
    StringList chNames = cov.names;
    if (chNames.empty() && !rawFile.isEmpty()) {
        FiffRawData raw(rawFile.toStdU16String());
        chNames = raw.info.ch_names;
    }

    // Eigendecomposition of the covariance matrix
    MatrixXd covMat = cov.data;
    SelfAdjointEigenSolver<MatrixXd> eig(covMat);
    if (eig.info() != Success) {
        qCritical("Eigendecomposition failed.");
        return 1;
    }

    VectorXd eigenvalues = eig.eigenvalues();
    MatrixXd eigenvectors = eig.eigenvectors();

    qInfo("Eigenvalue range: %g .. %g",
          eigenvalues(0), eigenvalues(eigenvalues.size() - 1));

    // Select the top nProj eigenvectors (largest eigenvalues = most noise)
    int nEig = eigenvalues.size();
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

        qInfo("  Projector %d: eigenvalue = %g", p + 1, eigenvalues(idx));
    }

    // Write projectors to file
    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly)) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }
    auto stream = FiffStream::start_file(outFile.toStdU16String());
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    stream->start_block(FIFFB_MNE);
    stream->write_proj(projs);
    stream->end_block(FIFFB_MNE);
    stream->end_file();

    qInfo("Written %d projectors to: %s", nProj, qPrintable(outFile));
    return 0;
}
