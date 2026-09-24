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

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
    QCoreApplication::setApplicationName("mne_average_forward_solutions");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Average multiple forward solutions.\n\nUse --fwd multiple times or --listfile to specify input files.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fwdOpt("fwd", "Forward solution FIFF file (repeat for each).", "file");
    parser.addOption(fwdOpt);

    QCommandLineOption weightsOpt("weights", "Comma-separated weights (default: equal).", "w1,w2,...");
    parser.addOption(weightsOpt);

    QCommandLineOption listfileOpt("listfile", "Text file with one fwd filename per line.", "file");
    parser.addOption(listfileOpt);

    QCommandLineOption outOpt("out", "Output averaged forward solution FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QStringList fwdFiles = parser.values(fwdOpt);
    QString weightStr = parser.value(weightsOpt);
    QString listFile = parser.value(listfileOpt);
    QString outFile = parser.value(outOpt);

    // Read files from listfile if specified
    if (!listFile.isEmpty()) {
        QFile lf(listFile);
        if (!lf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCritical("Cannot open list file: %s", qPrintable(listFile));
            return 1;
        }
        QTextStream in(&lf);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'))
                fwdFiles.append(line);
        }
    }

    if (fwdFiles.size() < 2) { qCritical("At least 2 forward solutions are required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Parse weights
    QList<double> weights;
    if (!weightStr.isEmpty()) {
        QStringList parts = weightStr.split(',');
        for (const QString& p : parts)
            weights.append(p.toDouble());
        if (weights.size() != fwdFiles.size()) {
            qCritical("Number of weights (%lld) doesn't match number of files (%lld)",
                      static_cast<long long>(weights.size()),
                      static_cast<long long>(fwdFiles.size()));
            return 1;
        }
    } else {
        // Equal weights
        for (int i = 0; i < fwdFiles.size(); ++i)
            weights.append(1.0);
    }

    // Normalize weights
    double wSum = 0;
    for (double w : weights) wSum += w;
    for (int i = 0; i < weights.size(); ++i) weights[i] /= wSum;

    // Load first forward solution as template
    qInfo("Loading %lld forward solutions..." , static_cast<long long>(fwdFiles.size()));

    QFile f0(fwdFiles[0]);
    MNEForwardSolution fwd0(f0);
    if (fwd0.sol->data.size() == 0) {
        qCritical("Cannot read forward solution: %s", qPrintable(fwdFiles[0]));
        return 1;
    }
    qInfo("  [1] %s: %d channels x %d sources (weight=%.4f)" ,
           qPrintable(fwdFiles[0]), (int)fwd0.sol->data.rows(), (int)fwd0.sol->data.cols(), weights[0]);

    int nChan = fwd0.sol->data.rows();
    int nSrc = fwd0.sol->data.cols();

    // Weighted sum
    MatrixXd avgSol = weights[0] * fwd0.sol->data;

    for (int i = 1; i < fwdFiles.size(); ++i) {
        QFile fi(fwdFiles[i]);
        MNEForwardSolution fwdi(fi);
        if (fwdi.sol->data.size() == 0) {
            qCritical("Cannot read forward solution: %s", qPrintable(fwdFiles[i]));
            return 1;
        }

        if (fwdi.sol->data.rows() != nChan || fwdi.sol->data.cols() != nSrc) {
            qCritical("Forward solution dimension mismatch in %s: %dx%d vs %dx%d",
                      qPrintable(fwdFiles[i]),
                      (int)fwdi.sol->data.rows(), (int)fwdi.sol->data.cols(),
                      nChan, nSrc);
            return 1;
        }

        qInfo("  [%d] %s: %d channels x %d sources (weight=%.4f)" ,
               i + 1, qPrintable(fwdFiles[i]), nChan, nSrc, weights[i]);

        avgSol += weights[i] * fwdi.sol->data;
    }

    qInfo("Averaged forward solution: %d channels x %d sources" , nChan, nSrc);

    // Write averaged forward solution
    fwd0.sol->data = avgSol;

    QFile outF(outFile);
    fwd0.write(outF);

    qInfo("Written averaged forward solution to: %s" , qPrintable(outFile));
    return 0;
}
