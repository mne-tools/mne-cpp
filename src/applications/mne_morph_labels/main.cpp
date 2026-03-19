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
 * @brief    Morph label files between subjects using sphere-registered surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_surface.h>
#include <fs/fs_label.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
/**
 * For each vertex on the source sphere, find the nearest vertex on the destination sphere.
 * This is the core of the morphing: vertices are matched by proximity on the sphere registration.
 */
static VectorXi buildNearestMap(const MatrixX3f& srcSphere, const MatrixX3f& dstSphere)
{
    int nSrc = srcSphere.rows();
    int nDst = dstSphere.rows();
    VectorXi mapping(nSrc);

    for (int s = 0; s < nSrc; ++s) {
        float minDist = std::numeric_limits<float>::max();
        int nearest = 0;
        for (int d = 0; d < nDst; ++d) {
            float dist = (srcSphere.row(s) - dstSphere.row(d)).squaredNorm();
            if (dist < minDist) {
                minDist = dist;
                nearest = d;
            }
        }
        mapping(s) = nearest;
    }
    return mapping;
}

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Morph label files between subjects using sphere registration.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --from <subject>     Source subject name\n");
    fprintf(stderr, "  --to <subject>       Destination subject name\n");
    fprintf(stderr, "  --subjects_dir <dir> Subjects directory (default: $SUBJECTS_DIR)\n");
    fprintf(stderr, "  --hemi <hemi>        Hemisphere: lh or rh\n");
    fprintf(stderr, "  --label <file>       Input label file\n");
    fprintf(stderr, "  --out <file>         Output morphed label file\n");
    fprintf(stderr, "  --help               Print this help\n");
    fprintf(stderr, "  --version            Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString fromSubject;
    QString toSubject;
    QString subjectsDir = qEnvironmentVariable("SUBJECTS_DIR");
    QString hemi;
    QString labelFile;
    QString outFile;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--from") == 0) {
            if (++k >= argc) { qCritical("--from: argument required."); return 1; }
            fromSubject = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--to") == 0) {
            if (++k >= argc) { qCritical("--to: argument required."); return 1; }
            toSubject = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--subjects_dir") == 0) {
            if (++k >= argc) { qCritical("--subjects_dir: argument required."); return 1; }
            subjectsDir = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--hemi") == 0) {
            if (++k >= argc) { qCritical("--hemi: argument required."); return 1; }
            hemi = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--label") == 0) {
            if (++k >= argc) { qCritical("--label: argument required."); return 1; }
            labelFile = QString(argv[k]);
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

    if (fromSubject.isEmpty() || toSubject.isEmpty()) {
        qCritical("--from and --to are required."); usage(argv[0]); return 1;
    }
    if (hemi.isEmpty()) { qCritical("--hemi is required."); usage(argv[0]); return 1; }
    if (labelFile.isEmpty()) { qCritical("--label is required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }
    if (subjectsDir.isEmpty()) { qCritical("$SUBJECTS_DIR not set."); return 1; }

    // Load sphere-registered surfaces
    QString srcSpherePath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, fromSubject, hemi);
    QString dstSpherePath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, toSubject, hemi);
    QString dstWhitePath = QString("%1/%2/surf/%3.white").arg(subjectsDir, toSubject, hemi);

    FsSurface srcSphere, dstSphere, dstWhite;
    if (!FsSurface::read(srcSpherePath, srcSphere)) {
        qCritical("Cannot read source sphere: %s", qPrintable(srcSpherePath));
        return 1;
    }
    if (!FsSurface::read(dstSpherePath, dstSphere)) {
        qCritical("Cannot read dest sphere: %s", qPrintable(dstSpherePath));
        return 1;
    }
    if (!FsSurface::read(dstWhitePath, dstWhite)) {
        qCritical("Cannot read dest white surface: %s", qPrintable(dstWhitePath));
        return 1;
    }

    printf("Source sphere: %d vertices\n", (int)srcSphere.rr().rows());
    printf("Dest sphere:   %d vertices\n", (int)dstSphere.rr().rows());

    // Build nearest-neighbor mapping on sphere
    printf("Building nearest-neighbor mapping on sphere...\n");
    VectorXi mapping = buildNearestMap(srcSphere.rr(), dstSphere.rr());

    // Read input label
    FsLabel label;
    if (!FsLabel::read(labelFile, label)) {
        qCritical("Cannot read label: %s", qPrintable(labelFile));
        return 1;
    }
    printf("Input label: %d vertices\n", (int)label.vertices.size());

    // Morph label vertices
    std::set<int> morphedSet;
    for (int i = 0; i < label.vertices.size(); ++i) {
        int srcVert = label.vertices(i);
        if (srcVert >= 0 && srcVert < mapping.size()) {
            morphedSet.insert(mapping(srcVert));
        }
    }

    // Write morphed label
    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot write: %s", qPrintable(outFile));
        return 1;
    }
    QTextStream out(&outF);
    out << "#Morphed from " << fromSubject << " to " << toSubject << "\n";
    out << morphedSet.size() << "\n";
    for (int v : morphedSet) {
        if (v < dstWhite.rr().rows()) {
            out << QString::asprintf("%d  %8.4f  %8.4f  %8.4f  %10.6f\n",
                v,
                dstWhite.rr()(v, 0), dstWhite.rr()(v, 1), dstWhite.rr()(v, 2),
                0.0);
        }
    }
    outF.close();

    printf("Morphed label: %d vertices -> %s\n", (int)morphedSet.size(), qPrintable(outFile));
    return 0;
}
