//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Compute sensitivity maps from a forward solution using SVD of the gain matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_forward_solution.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>

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

    if (fwdFile.isEmpty()) {
        Log::critical("--fwd is required.");
        parser.showHelp(1);
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        parser.showHelp(1);
    }

    // Load forward solution
    auto fwdRead = MNEForwardSolution::read(fwdFile.toStdU16String());
    if (!fwdRead) {
        Log::critical("{}", fwdRead.error().what());
        return 1;
    }
    MNEForwardSolution& fwd = *fwdRead;

    MatrixXd G = fwd.sol.data;
    int nChan = G.rows();
    int nSrc = G.cols();
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
        Log::critical("Unknown method: {} (use 'norm' or 'svd')", method.toStdString());
        return 1;
    }

    // Normalize to [0, 1]
    double maxSens = sensitivity.maxCoeff();
    if (maxSens > 0)
        sensitivity /= maxSens;

    Log::info("Sensitivity range: {:g} .. {:g} (normalized)", sensitivity.minCoeff(), sensitivity.maxCoeff());

    // Write output
    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::critical("Cannot write: {}", outFile.toStdString());
        return 1;
    }
    QTextStream out(&outF);
    out << "# Sensitivity map (" << method << " method)\n";
    out << "# " << nSourcePoints << " sources\n";
    for (int s = 0; s < nSourcePoints; ++s)
        out << s << " " << QString::number(sensitivity(s), 'g', 10) << "\n";
    outF.close();

    Log::info("Written sensitivity map to: {}", outFile.toStdString());
    return 0;
}
