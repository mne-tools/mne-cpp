//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Generate decimated scalp surfaces from BEM outer skin.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_types.h>
#include <base/log.h>
#include <base/log_qt.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <iterator>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

int main(int argc, char* argv[])
{
    mne::install_qt_message_handler();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_scalp_surfaces");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Generate decimated scalp surfaces from BEM outer skin.\n\n"
                                     "Reads a BEM file, finds the outer skin surface, and creates\n"
                                     "decimated versions at the specified vertex counts.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bemOpt("bem", "BEM file with outer skin surface.", "file");
    parser.addOption(bemOpt);

    QCommandLineOption outOpt("out", "Output directory for decimated surfaces.", "dir");
    parser.addOption(outOpt);

    QCommandLineOption gradesOpt("grades", "Comma-separated target vertex counts (default: 2562,10242,40962).",
                                 "counts", "2562,10242,40962");
    parser.addOption(gradesOpt);

    parser.process(app);

    QString bemFile = parser.value(bemOpt);
    QString outDir = parser.value(outOpt);
    QString gradesStr = parser.value(gradesOpt);

    if (bemFile.isEmpty()) {
        qCritical("--bem is required.");
        return 1;
    }
    if (outDir.isEmpty()) {
        qCritical("--out is required.");
        return 1;
    }

    // Parse grades
    QStringList gradesParts = gradesStr.split(',', Qt::SkipEmptyParts);
    QList<int> grades;
    for (const QString& g : gradesParts) {
        int val = g.trimmed().toInt();
        if (val > 0)
            grades.append(val);
    }
    if (grades.isEmpty()) {
        qCritical("No valid vertex counts specified.");
        return 1;
    }

    const QFileInfo bemFileInfo(bemFile);
    if (!bemFileInfo.isFile() || !bemFileInfo.isReadable()) {
        qCritical("Cannot read BEM from: %s", qPrintable(bemFile));
        return 1;
    }

    // Read BEM
    MNEBem bem(bemFile.toStdU16String());
    if (bem.size() == 0) {
        qCritical("Cannot read BEM from: %s", qPrintable(bemFile));
        return 1;
    }

    qInfo("Read BEM with %zu surface(s) from %s", bem.size(), qPrintable(bemFile));

    // Find outer skin surface (typically the last/outermost surface, or id == FIFFV_BEM_SURF_ID_HEAD)
    auto head = std::ranges::find(bem, FIFFV_BEM_SURF_ID_HEAD, &MNEBemSurface::id);
    if (head == bem.end()) {
        head = std::prev(bem.end());
        qInfo("No explicit outer skin surface found, using the outermost surface.");
    }
    const MNEBemSurface& skin = *head;
    qInfo("Using surface %td: %d vertices, %d triangles", head - bem.begin(), skin.np, skin.ntri);

    // Create output directory
    QDir dir;
    if (!dir.mkpath(outDir)) {
        qCritical("Cannot create output directory: %s", qPrintable(outDir));
        return 1;
    }

    // Generate decimated surfaces
    for (int targetVerts : grades) {
        qInfo("Decimating to %d vertices...", targetVerts);
        const std::vector<MNEBemSurface> surfaces =
            MNEBemSurface::makeScalpSurfaces(skin, {targetVerts});
        if (surfaces.empty()) {
            qCritical("Could not decimate scalp surface to %d vertices.", targetVerts);
            return 1;
        }
        const MNEBemSurface& decimated = surfaces.front();

        // Write as FIFF BEM surface
        QString outPath = QDir(outDir).filePath(
            QString("scalp-%1.fif").arg(targetVerts));

        MNEBem outBem;
        outBem.push_back(decimated);

        if (auto written = outBem.write(outPath.toStdU16String()); !written) {
            qCritical("%s", written.error().what());
            return 1;
        }

        qInfo("  Written %d vertices, %d triangles to %s",
              decimated.np, decimated.ntri, qPrintable(outPath));
    }

    qInfo("Done.");
    return 0;
}
