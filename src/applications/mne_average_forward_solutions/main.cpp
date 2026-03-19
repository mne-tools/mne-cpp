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
 * @brief    Average multiple forward solutions (weighted by number of averages).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
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
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Average multiple forward solutions.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --fwd <file>       Forward solution FIFF file (repeat for each)\n");
    fprintf(stderr, "  --weights <w1,w2,...>  Comma-separated weights (default: equal)\n");
    fprintf(stderr, "  --listfile <file>  Text file with one fwd filename per line\n");
    fprintf(stderr, "  --out <file>       Output averaged forward solution FIFF file\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList fwdFiles;
    QString weightStr;
    QString listFile;
    QString outFile;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--fwd") == 0) {
            if (++k >= argc) { qCritical("--fwd: argument required."); return 1; }
            fwdFiles.append(QString(argv[k]));
        }
        else if (strcmp(argv[k], "--weights") == 0) {
            if (++k >= argc) { qCritical("--weights: argument required."); return 1; }
            weightStr = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--listfile") == 0) {
            if (++k >= argc) { qCritical("--listfile: argument required."); return 1; }
            listFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

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

    if (fwdFiles.size() < 2) { qCritical("At least 2 forward solutions are required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }

    // Parse weights
    QList<double> weights;
    if (!weightStr.isEmpty()) {
        QStringList parts = weightStr.split(',');
        for (const QString& p : parts)
            weights.append(p.toDouble());
        if (weights.size() != fwdFiles.size()) {
            qCritical("Number of weights (%d) doesn't match number of files (%d)",
                      weights.size(), fwdFiles.size());
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
    printf("Loading %d forward solutions...\n", fwdFiles.size());

    QFile f0(fwdFiles[0]);
    MNEForwardSolution fwd0(f0);
    if (fwd0.sol->data.size() == 0) {
        qCritical("Cannot read forward solution: %s", qPrintable(fwdFiles[0]));
        return 1;
    }
    printf("  [1] %s: %d channels x %d sources (weight=%.4f)\n",
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

        printf("  [%d] %s: %d channels x %d sources (weight=%.4f)\n",
               i + 1, qPrintable(fwdFiles[i]), nChan, nSrc, weights[i]);

        avgSol += weights[i] * fwdi.sol->data;
    }

    printf("Averaged forward solution: %d channels x %d sources\n", nChan, nSrc);

    // Write averaged forward solution
    fwd0.sol->data = avgSol;

    QFile outF(outFile);
    fwd0.write(outF);

    printf("Written averaged forward solution to: %s\n", qPrintable(outFile));
    return 0;
}
