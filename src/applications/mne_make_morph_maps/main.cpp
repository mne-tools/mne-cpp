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
#include <utils/generics/applicationlogger.h>

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

#define PROGRAM_VERSION "2.0.0"
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
    triplets.reserve(nDst * nNearest);

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
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
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
    outF.open(QIODevice::WriteOnly);
    FiffStream::SPtr stream = FiffStream::start_file(outF);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }
    stream->start_block(FIFFB_MNE);

    for (const QString& hemi : hemis) {
        printf("\nProcessing %s hemisphere...\n", qPrintable(hemi));

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

        printf("  Source: %d vertices\n", (int)srcSphere.rr().rows());
        printf("  Dest:   %d vertices\n", (int)dstSphere.rr().rows());

        // Compute morph map
        printf("  Computing morph map (nearest=%d)...\n", nNearest);
        SparseMatrix<double> morphMap = computeMorphMap(srcSphere.rr(), dstSphere.rr(), nNearest);
        printf("  Morph map: %dx%d, %ld nonzeros\n",
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

    printf("\nWritten morph maps to: %s\n", qPrintable(outFile));
    return 0;
}
