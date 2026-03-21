//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Convert noise covariance matrix eigenvectors to SSP projection operators.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_raw_data.h>
#include <utils/generics/mne_logger.h>

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

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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

    if (covFile.isEmpty()) { qCritical("--cov is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Read covariance
    QFile fCov(covFile);
    FiffCov cov(fCov);
    if (cov.isEmpty()) {
        qCritical("Cannot read covariance from: %s", qPrintable(covFile));
        return 1;
    }
    printf("Read covariance: %d x %d channels\n", cov.dim, cov.dim);

    // Get channel names
    QStringList chNames = cov.names;
    if (chNames.isEmpty() && !rawFile.isEmpty()) {
        QFile fRaw(rawFile);
        FiffRawData raw(fRaw);
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

    printf("Eigenvalue range: %g .. %g\n",
           eigenvalues(0), eigenvalues(eigenvalues.size() - 1));

    // Select the top nProj eigenvectors (largest eigenvalues = most noise)
    int nEig = eigenvalues.size();
    if (nProj > nEig) nProj = nEig;

    // Eigenvectors are in ascending order; take from the end
    QList<FiffProj> projs;
    for (int p = 0; p < nProj; ++p) {
        int idx = nEig - 1 - p;
        FiffProj proj;
        proj.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF; // Generic projection
        proj.active = false;
        proj.desc = QString("Cov eigenvector #%1 (eigenvalue=%2)")
                        .arg(p + 1).arg(eigenvalues(idx), 0, 'g', 6);

        // Build the named matrix for this projector
        FiffNamedMatrix::SDPtr data = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
        data->nrow = 1;
        data->ncol = nEig;
        data->col_names = chNames;
        data->data = eigenvectors.col(idx).transpose();

        proj.data = data;
        projs.append(proj);

        printf("  Projector %d: eigenvalue = %g\n", p + 1, eigenvalues(idx));
    }

    // Write projectors to file
    QFile outF(outFile);
    outF.open(QIODevice::WriteOnly);
    FiffStream::SPtr stream = FiffStream::start_file(outF);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    stream->start_block(FIFFB_MNE);
    stream->write_proj(projs);
    stream->end_block(FIFFB_MNE);
    stream->end_file();

    printf("Written %d projectors to: %s\n", nProj, qPrintable(outFile));
    return 0;
}
