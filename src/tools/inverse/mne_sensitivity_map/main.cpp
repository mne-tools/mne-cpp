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
 * @brief    Compute sensitivity maps from a forward solution using SVD of the gain matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
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
    QCoreApplication::setApplicationName("mne_sensitivity_map");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compute sensitivity maps from forward solution gain matrix.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fwdOpt("fwd", "Forward solution FIFF file.", "file");
    parser.addOption(fwdOpt);

    QCommandLineOption outOpt("out", "Output sensitivity map (text format).", "file");
    parser.addOption(outOpt);

    QCommandLineOption methodOpt("method", "Method: 'norm' (column norms) or 'svd' (leading singular value).", "name", "norm");
    parser.addOption(methodOpt);

    parser.process(app);

    QString fwdFile = parser.value(fwdOpt);
    QString outFile = parser.value(outOpt);
    QString method = parser.value(methodOpt).toLower();

    if (fwdFile.isEmpty()) { qCritical("--fwd is required."); parser.showHelp(1); }
    if (outFile.isEmpty()) { qCritical("--out is required."); parser.showHelp(1); }

    // Load forward solution
    QFile f(fwdFile);
    MNEForwardSolution fwd(f);
    if (fwd.sol->data.size() == 0) {
        qCritical("Cannot read forward solution: %s", qPrintable(fwdFile));
        return 1;
    }

    MatrixXd G = fwd.sol->data;
    int nChan = G.rows();
    int nSrc = G.cols();
    bool isFixed = fwd.isFixedOrient();

    printf("Forward: %d channels x %d sources (%s orientation)\n",
           nChan, nSrc, isFixed ? "fixed" : "free");

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
        qCritical("Unknown method: %s (use 'norm' or 'svd')", qPrintable(method));
        return 1;
    }

    // Normalize to [0, 1]
    double maxSens = sensitivity.maxCoeff();
    if (maxSens > 0) sensitivity /= maxSens;

    printf("Sensitivity range: %g .. %g (normalized)\n",
           sensitivity.minCoeff(), sensitivity.maxCoeff());

    // Write output
    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot write: %s", qPrintable(outFile));
        return 1;
    }
    QTextStream out(&outF);
    out << "# Sensitivity map (" << method << " method)\n";
    out << "# " << nSourcePoints << " sources\n";
    for (int s = 0; s < nSourcePoints; ++s)
        out << s << " " << QString::number(sensitivity(s), 'g', 10) << "\n";
    outF.close();

    printf("Written sensitivity map to: %s\n", qPrintable(outFile));
    return 0;
}
