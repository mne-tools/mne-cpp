//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Smooth source estimates on a cortical surface using iterative Laplacian averaging.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_spaces.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fs/fs_surface.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

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
        neighbors[v0].insert(v1);
        neighbors[v0].insert(v2);
        neighbors[v1].insert(v0);
        neighbors[v1].insert(v2);
        neighbors[v2].insert(v0);
        neighbors[v2].insert(v1);
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

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_smooth");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Smooth source estimate data on a cortical surface.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption srcOpt("src", "Source space FIFF file (for surface connectivity).", "file");
    parser.addOption(srcOpt);

    QCommandLineOption surfOpt("surf", "FreeSurfer surface file (alternative to --src).", "file");
    parser.addOption(surfOpt);

    QCommandLineOption stcOpt("stc", "Input STC file (text format: vertex value [per time]).", "file");
    parser.addOption(stcOpt);

    QCommandLineOption outOpt("out", "Output smoothed STC file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption smoothOpt("smooth", "Number of smoothing iterations.", "n", "5");
    parser.addOption(smoothOpt);

    parser.process(app);

    QString srcFile = parser.value(srcOpt);
    QString surfFile = parser.value(surfOpt);
    QString stcFile = parser.value(stcOpt);
    QString outFile = parser.value(outOpt);
    int nSmooth = parser.value(smoothOpt).toInt();

    if (srcFile.isEmpty() && surfFile.isEmpty()) {
        Log::critical("Either --src or --surf is required.");
        parser.showHelp(1);
    }
    if (stcFile.isEmpty()) {
        Log::critical("--stc is required.");
        parser.showHelp(1);
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        parser.showHelp(1);
    }

    // Get surface triangulation
    MatrixX3i tris;
    int nVert = 0;

    if (!surfFile.isEmpty()) {
        FsSurface surface;
        if (!FsSurface::read(surfFile.toStdU16String(), surface)) {
            Log::critical("Cannot read surface: {}", surfFile.toStdString());
            return 1;
        }
        tris = surface.tris();
        nVert = surface.rr().rows();
        Log::info("Read surface: {} vertices, {} triangles", nVert, static_cast<int>(tris.rows()));
    } else {
        // Read source space
        auto read = MNESourceSpaces::read(srcFile.toStdU16String());
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        const MNESourceSpaces& srcSpaces = *read;
        // Use first hemisphere's triangulation
        tris = srcSpaces[0].itris;
        nVert = srcSpaces[0].np;
        Log::info("Read source space: {} vertices, {} triangles", nVert, static_cast<int>(tris.rows()));
    }

    // Build smoothing operator
    SparseMatrix<double> S = buildSmoothingOperator(tris, nVert);
    Log::info("Built smoothing operator: {} x {}", static_cast<int>(S.rows()), static_cast<int>(S.cols()));

    // Read STC data (text format: one value per line, or vertex value pairs)
    QFile stcIn(stcFile);
    if (!stcIn.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open STC file: {}", stcFile.toStdString());
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
    Log::info("Read {} data values", static_cast<int>(dataList.size()));

    // Apply iterative smoothing
    VectorXd smoothed = data;
    for (int iter = 0; iter < nSmooth; ++iter) {
        smoothed = S * smoothed;
    }
    Log::info("Applied {} iterations of Laplacian smoothing", nSmooth);

    // Write output
    QFile stcOut(outFile);
    if (!stcOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::critical("Cannot write output file: {}", outFile.toStdString());
        return 1;
    }
    QTextStream out(&stcOut);
    out << "# Smoothed source estimate (" << nSmooth << " iterations)\n";
    for (int i = 0; i < nVert; ++i) {
        if (smoothed(i) != 0.0)
            out << i << " " << QString::number(smoothed(i), 'g', 10) << "\n";
    }
    stcOut.close();
    Log::info("Written smoothed data to: {}", outFile.toStdString());

    return 0;
}
