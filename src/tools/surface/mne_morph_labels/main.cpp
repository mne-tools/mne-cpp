//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Morph label files between subjects using sphere-registered surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_label.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_morph_labels");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Morph label files between subjects using sphere registration.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source subject name.", "subject");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Destination subject name.", "subject");
    parser.addOption(toOpt);

    QCommandLineOption subjectsDirOpt("subjects_dir", "Subjects directory.", "dir",
                                      qEnvironmentVariable("SUBJECTS_DIR"));
    parser.addOption(subjectsDirOpt);

    QCommandLineOption hemiOpt("hemi", "Hemisphere: lh or rh.", "hemi");
    parser.addOption(hemiOpt);

    QCommandLineOption labelOpt("label", "Input label file.", "file");
    parser.addOption(labelOpt);

    QCommandLineOption outOpt("out", "Output morphed label file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString fromSubject = parser.value(fromOpt);
    QString toSubject = parser.value(toOpt);
    QString subjectsDir = parser.value(subjectsDirOpt);
    QString hemi = parser.value(hemiOpt);
    QString labelFile = parser.value(labelOpt);
    QString outFile = parser.value(outOpt);

    if (fromSubject.isEmpty() || toSubject.isEmpty()) {
        Log::critical("--from and --to are required.");
        parser.showHelp(1);
    }
    if (hemi.isEmpty()) {
        Log::critical("--hemi is required.");
        parser.showHelp(1);
    }
    if (labelFile.isEmpty()) {
        Log::critical("--label is required.");
        parser.showHelp(1);
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        parser.showHelp(1);
    }
    if (subjectsDir.isEmpty()) {
        Log::critical("$SUBJECTS_DIR not set.");
        return 1;
    }

    // Load sphere-registered surfaces
    QString srcSpherePath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, fromSubject, hemi);
    QString dstSpherePath = QString("%1/%2/surf/%3.sphere.reg").arg(subjectsDir, toSubject, hemi);
    QString dstWhitePath = QString("%1/%2/surf/%3.white").arg(subjectsDir, toSubject, hemi);

    FsSurface srcSphere, dstSphere, dstWhite;
    if (!FsSurface::read(srcSpherePath.toStdU16String(), srcSphere)) {
        Log::critical("Cannot read source sphere: {}", srcSpherePath.toStdString());
        return 1;
    }
    if (!FsSurface::read(dstSpherePath.toStdU16String(), dstSphere)) {
        Log::critical("Cannot read dest sphere: {}", dstSpherePath.toStdString());
        return 1;
    }
    if (!FsSurface::read(dstWhitePath.toStdU16String(), dstWhite)) {
        Log::critical("Cannot read dest white surface: {}", dstWhitePath.toStdString());
        return 1;
    }

    Log::info("Source sphere: {} vertices", static_cast<int>(srcSphere.rr().rows()));
    Log::info("Dest sphere:   {} vertices", static_cast<int>(dstSphere.rr().rows()));

    // Build nearest-neighbor mapping on sphere
    Log::info("Building nearest-neighbor mapping on sphere...");
    VectorXi mapping = buildNearestMap(srcSphere.rr(), dstSphere.rr());

    // Read input label
    FsLabel label;
    if (!FsLabel::read(labelFile.toStdU16String(), label)) {
        Log::critical("Cannot read label: {}", labelFile.toStdString());
        return 1;
    }
    Log::info("Input label: {} vertices", static_cast<int>(label.vertices.size()));

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
        Log::critical("Cannot write: {}", outFile.toStdString());
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

    Log::info("Morphed label: {} vertices -> {}", static_cast<int>(morphedSet.size()), outFile.toStdString());
    return 0;
}
