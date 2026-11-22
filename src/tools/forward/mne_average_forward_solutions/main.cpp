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
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
            Log::critical("Cannot open list file: {}", listFile.toStdString());
            return 1;
        }
        QTextStream in(&lf);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'))
                fwdFiles.append(line);
        }
    }

    if (fwdFiles.size() < 2) {
        Log::critical("At least 2 forward solutions are required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Parse weights
    QList<double> weights;
    if (!weightStr.isEmpty()) {
        QStringList parts = weightStr.split(',');
        for (const QString& p : parts)
            weights.append(p.toDouble());
        if (weights.size() != fwdFiles.size()) {
            Log::critical("Number of weights ({}) doesn't match number of files ({})", static_cast<long long>(weights.size()), static_cast<long long>(fwdFiles.size()));
            return 1;
        }
    } else {
        // Equal weights
        for (int i = 0; i < fwdFiles.size(); ++i)
            weights.append(1.0);
    }

    // Normalize weights
    double wSum = 0;
    for (double w : weights)
        wSum += w;
    for (int i = 0; i < weights.size(); ++i)
        weights[i] /= wSum;

    // Load first forward solution as template
    Log::info("Loading {} forward solutions...", static_cast<long long>(fwdFiles.size()));

    auto fwd0Read = MNEForwardSolution::read(fwdFiles[0].toStdU16String());
    if (!fwd0Read) {
        Log::critical("{}", fwd0Read.error().what());
        return 1;
    }
    MNEForwardSolution& fwd0 = *fwd0Read;
    Log::info("  [1] {}: {} channels x {} sources (weight={:.4f})", fwdFiles[0].toStdString(), static_cast<int>(fwd0.sol.data.rows()), static_cast<int>(fwd0.sol.data.cols()), weights[0]);

    int nChan = fwd0.sol.data.rows();
    int nSrc = fwd0.sol.data.cols();

    // Weighted sum
    MatrixXd avgSol = weights[0] * fwd0.sol.data;

    for (int i = 1; i < fwdFiles.size(); ++i) {
        auto fwdiRead = MNEForwardSolution::read(fwdFiles[i].toStdU16String());
        if (!fwdiRead) {
            Log::critical("{}", fwdiRead.error().what());
            return 1;
        }
        MNEForwardSolution& fwdi = *fwdiRead;

        if (fwdi.sol.data.rows() != nChan || fwdi.sol.data.cols() != nSrc) {
            Log::critical("Forward solution dimension mismatch in {}: {}x{} vs {}x{}", fwdFiles[i].toStdString(), static_cast<int>(fwdi.sol.data.rows()), static_cast<int>(fwdi.sol.data.cols()), nChan, nSrc);
            return 1;
        }

        Log::info("  [{}] {}: {} channels x {} sources (weight={:.4f})", i + 1, fwdFiles[i].toStdString(), nChan, nSrc, weights[i]);

        avgSol += weights[i] * fwdi.sol.data;
    }

    Log::info("Averaged forward solution: {} channels x {} sources", nChan, nSrc);

    // Write averaged forward solution
    fwd0.sol.data = avgSol;

    if (auto written = fwd0.write(outFile.toStdU16String()); !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }

    Log::info("Written averaged forward solution to: {}", outFile.toStdString());
    return 0;
}
