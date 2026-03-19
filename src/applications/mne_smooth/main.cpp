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
 * @brief    Smooth source estimates on a cortical surface using iterative Laplacian averaging.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_spaces.h>
#include <mne/mne_source_space.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <fs/fs_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
/**
 * Build an adjacency-based smoothing operator from surface triangulation.
 * Returns a sparse matrix S where S(i,j) = 1/N_i for each neighbor j of i,
 * and S(i,i) = 0 (pure averaging of neighbors).
 */
static SparseMatrix<double> buildSmoothingOperator(const MatrixX3i& tris, int nVert)
{
    // Count neighbors for each vertex
    VectorXi degree = VectorXi::Zero(nVert);
    std::vector<std::set<int>> neighbors(nVert);

    for (int t = 0; t < tris.rows(); ++t) {
        int v0 = tris(t, 0), v1 = tris(t, 1), v2 = tris(t, 2);
        neighbors[v0].insert(v1); neighbors[v0].insert(v2);
        neighbors[v1].insert(v0); neighbors[v1].insert(v2);
        neighbors[v2].insert(v0); neighbors[v2].insert(v1);
    }

    // Build sparse triplets
    typedef Triplet<double> T;
    std::vector<T> triplets;
    triplets.reserve(nVert * 7); // ~6 neighbors average on icosphere

    for (int i = 0; i < nVert; ++i) {
        int deg = static_cast<int>(neighbors[i].size());
        if (deg > 0) {
            double w = 1.0 / deg;
            for (int j : neighbors[i])
                triplets.push_back(T(i, j, w));
        } else {
            // Isolated vertex: keep itself
            triplets.push_back(T(i, i, 1.0));
        }
    }

    SparseMatrix<double> S(nVert, nVert);
    S.setFromTriplets(triplets.begin(), triplets.end());
    return S;
}

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Smooth source estimate data on a cortical surface.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --src <file>       Source space FIFF file (for surface connectivity)\n");
    fprintf(stderr, "  --surf <file>      FreeSurfer surface file (alternative to --src)\n");
    fprintf(stderr, "  --stc <file>       Input STC file (text format: vertex value [per time])\n");
    fprintf(stderr, "  --out <file>       Output smoothed STC file\n");
    fprintf(stderr, "  --smooth <n>       Number of smoothing iterations (default: 5)\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString srcFile;
    QString surfFile;
    QString stcFile;
    QString outFile;
    int nSmooth = 5;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--src") == 0) {
            if (++k >= argc) { qCritical("--src: argument required."); return 1; }
            srcFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--surf") == 0) {
            if (++k >= argc) { qCritical("--surf: argument required."); return 1; }
            surfFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--stc") == 0) {
            if (++k >= argc) { qCritical("--stc: argument required."); return 1; }
            stcFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--smooth") == 0) {
            if (++k >= argc) { qCritical("--smooth: argument required."); return 1; }
            nSmooth = atoi(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (srcFile.isEmpty() && surfFile.isEmpty()) {
        qCritical("Either --src or --surf is required.");
        usage(argv[0]);
        return 1;
    }
    if (stcFile.isEmpty()) { qCritical("--stc is required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }

    // Get surface triangulation
    MatrixX3i tris;
    int nVert = 0;

    if (!surfFile.isEmpty()) {
        FsSurface surface;
        if (!FsSurface::read(surfFile, surface)) {
            qCritical("Cannot read surface: %s", qPrintable(surfFile));
            return 1;
        }
        tris = surface.tris();
        nVert = surface.rr().rows();
        printf("Read surface: %d vertices, %d triangles\n", nVert, (int)tris.rows());
    } else {
        // Read source space
        QFile file(srcFile);
        file.open(QIODevice::ReadOnly);
        FiffStream::SPtr stream(new FiffStream(&file));
        MNESourceSpaces srcSpaces;
        if (!MNESourceSpaces::readFromStream(stream, true, srcSpaces) || srcSpaces.size() == 0) {
            qCritical("Cannot read source space: %s", qPrintable(srcFile));
            return 1;
        }
        // Use first hemisphere's triangulation
        tris = srcSpaces[0].itris;
        nVert = srcSpaces[0].np;
        printf("Read source space: %d vertices, %d triangles\n", nVert, (int)tris.rows());
    }

    // Build smoothing operator
    SparseMatrix<double> S = buildSmoothingOperator(tris, nVert);
    printf("Built smoothing operator: %d x %d\n", (int)S.rows(), (int)S.cols());

    // Read STC data (text format: one value per line, or vertex value pairs)
    QFile stcIn(stcFile);
    if (!stcIn.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open STC file: %s", qPrintable(stcFile));
        return 1;
    }

    QTextStream in(&stcIn);
    QList<QPair<int, double>> dataList;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 2) {
            dataList.append(qMakePair(parts[0].toInt(), parts[1].toDouble()));
        } else if (parts.size() == 1) {
            dataList.append(qMakePair(dataList.size(), parts[0].toDouble()));
        }
    }
    stcIn.close();

    // Build data vector
    VectorXd data = VectorXd::Zero(nVert);
    for (const auto& pair : dataList) {
        if (pair.first >= 0 && pair.first < nVert)
            data(pair.first) = pair.second;
    }
    printf("Read %d data values\n", (int)dataList.size());

    // Apply iterative smoothing
    VectorXd smoothed = data;
    for (int iter = 0; iter < nSmooth; ++iter) {
        smoothed = S * smoothed;
    }
    printf("Applied %d iterations of Laplacian smoothing\n", nSmooth);

    // Write output
    QFile stcOut(outFile);
    if (!stcOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }
    QTextStream out(&stcOut);
    out << "# Smoothed source estimate (" << nSmooth << " iterations)\n";
    for (int i = 0; i < nVert; ++i) {
        if (smoothed(i) != 0.0)
            out << i << " " << QString::number(smoothed(i), 'g', 10) << "\n";
    }
    stcOut.close();
    printf("Written smoothed data to: %s\n", qPrintable(outFile));

    return 0;
}
