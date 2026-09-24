//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Compute morphing maps between two subjects using sphere-registered surfaces.
 *
 * The morphing map provides a sparse matrix that transforms vertex data from one
 * subject's cortical surface to another, using the FreeSurfer spherical registration
 * (?h.sphere.reg). For each destination vertex, the nearest source vertices are
 * found on the registered sphere and their contributions are weighted by inverse distance.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_surface.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION
#define N_NEAREST 5  // Number of nearest neighbors for interpolation

//=============================================================================================================
/**
 * Build a morph map (sparse matrix) from src sphere to dst sphere.
 * For each destination vertex, finds the N_NEAREST source vertices on the sphere
 * and computes inverse-distance weights.
 */
static SparseMatrix<double> computeMorphMap(const MatrixX3f& srcSphere,
                                            const MatrixX3f& dstSphere,
                                            int nNearest)
{
    int nSrc = srcSphere.rows();
    int nDst = dstSphere.rows();

    typedef Triplet<double> T;
    std::vector<T> triplets;
    triplets.reserve(static_cast<std::size_t>(nDst) * nNearest);

    for (int d = 0; d < nDst; ++d) {
        // Find nNearest closest source vertices
        std::vector<std::pair<float, int>> dists(nSrc);
        for (int s = 0; s < nSrc; ++s)
            dists[s] = std::make_pair((srcSphere.row(s) - dstSphere.row(d)).squaredNorm(), s);

        std::partial_sort(dists.begin(), dists.begin() + nNearest, dists.end());

        // Compute inverse-distance weights
        double wSum = 0;
        std::vector<std::pair<int, double>> neighbors;
        for (int n = 0; n < nNearest; ++n) {
            float dist = sqrtf(dists[n].first);
            double w = (dist > 1e-10f) ? 1.0 / dist : 1e10;
            neighbors.push_back(std::make_pair(dists[n].second, w));
            wSum += w;
        }

        // Normalize and store
        for (auto& [idx, w] : neighbors) {
            triplets.push_back(T(d, idx, w / wSum));
        }
    }

    SparseMatrix<double> morphMap(nDst, nSrc);
    morphMap.setFromTriplets(triplets.begin(), triplets.end());
    return morphMap;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_morph_maps");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compute morphing maps between two subjects.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source subject name.", "subject");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Destination subject name.", "subject");
    parser.addOption(toOpt);

    QCommandLineOption subjDirOpt("subjects_dir", "Subjects directory.", "dir", qEnvironmentVariable("SUBJECTS_DIR"));
    parser.addOption(subjDirOpt);

    QCommandLineOption outOpt("out", "Output morph map FIFF file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption nearestOpt("nearest", "Number of nearest neighbors.", "n", "5");
    parser.addOption(nearestOpt);

    parser.process(app);

    QString fromSubject = parser.value(fromOpt);
    QString toSubject = parser.value(toOpt);
    QString subjectsDir = parser.value(subjDirOpt);
    QString outFile = parser.value(outOpt);
    int nNearest = parser.value(nearestOpt).toInt();

    if (fromSubject.isEmpty() || toSubject.isEmpty()) {
        qCritical("--from and --to are required."); return 1;
    }
    if (subjectsDir.isEmpty()) { qCritical("$SUBJECTS_DIR not set."); return 1; }
    if (outFile.isEmpty()) {
        outFile = QString("%1/morph-maps/%2-%3-morph.fif").arg(subjectsDir, fromSubject, toSubject);
        QDir().mkpath(QString("%1/morph-maps").arg(subjectsDir));
    }

    QStringList hemis = {"lh", "rh"};

    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly)) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }
    FiffStream::SPtr stream = FiffStream::start_file(outF);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }
    stream->start_block(FIFFB_MNE);

    for (const QString& hemi : hemis) {
        qInfo("\nProcessing %s hemisphere..." , qPrintable(hemi));

        // Load sphere-registered surfaces
        QString srcPath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, fromSubject, hemi);
        QString dstPath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, toSubject, hemi);

        FsSurface srcSphere, dstSphere;
        if (!FsSurface::read(srcPath, srcSphere)) {
            qCritical("Cannot read: %s", qPrintable(srcPath));
            stream->end_block(FIFFB_MNE);
            stream->end_file();
            return 1;
        }
        if (!FsSurface::read(dstPath, dstSphere)) {
            qCritical("Cannot read: %s", qPrintable(dstPath));
            stream->end_block(FIFFB_MNE);
            stream->end_file();
            return 1;
        }

        qInfo("  Source: %d vertices" , (int)srcSphere.rr().rows());
        qInfo("  Dest:   %d vertices" , (int)dstSphere.rr().rows());

        // Compute morph map
        qInfo("  Computing morph map (nearest=%d)..." , nNearest);
        SparseMatrix<double> morphMap = computeMorphMap(srcSphere.rr(), dstSphere.rr(), nNearest);
        qInfo("  Morph map: %dx%d, %ld nonzeros" ,
               (int)morphMap.rows(), (int)morphMap.cols(), (long)morphMap.nonZeros());

        // Write morph map as FIFF sparse matrix
        // Store as row/col/data arrays
        stream->start_block(FIFFB_MNE_MORPH_MAP);

        // Write hemisphere id
        int hemiId = (hemi == "lh") ? FIFFV_MNE_SURF_LEFT_HEMI : FIFFV_MNE_SURF_RIGHT_HEMI;
        stream->write_int(FIFF_MNE_HEMI, &hemiId);

        // Write source subject
        stream->write_string(FIFF_MNE_MORPH_MAP_FROM, fromSubject);
        stream->write_string(FIFF_MNE_MORPH_MAP_TO, toSubject);

        // Write the morph map as a FIFF sparse matrix (RCS format)
        SparseMatrix<float> morphMapF = morphMap.cast<float>();
        morphMapF.makeCompressed();
        stream->write_float_sparse_rcs(FIFF_MNE_MORPH_MAP, morphMapF);

        stream->end_block(FIFFB_MNE_MORPH_MAP);
    }

    stream->end_block(FIFFB_MNE);
    stream->end_file();

    qInfo("\nWritten morph maps to: %s" , qPrintable(outFile));
    return 0;
}
