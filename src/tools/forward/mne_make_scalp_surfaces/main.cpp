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

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>

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
        Log::critical("--bem is required.");
        return 1;
    }
    if (outDir.isEmpty()) {
        Log::critical("--out is required.");
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
        Log::critical("No valid vertex counts specified.");
        return 1;
    }

    const QFileInfo bemFileInfo(bemFile);
    if (!bemFileInfo.isFile() || !bemFileInfo.isReadable()) {
        Log::critical("Cannot read BEM from: {}", bemFile.toStdString());
        return 1;
    }

    // Read BEM
    MNEBem bem(bemFile.toStdU16String());
    if (bem.size() == 0) {
        Log::critical("Cannot read BEM from: {}", bemFile.toStdString());
        return 1;
    }

    Log::info("Read BEM with {} surface(s) from {}", bem.size(), bemFile.toStdString());

    // Find outer skin surface (typically the last/outermost surface, or id == FIFFV_BEM_SURF_ID_HEAD)
    auto head = std::ranges::find(bem, FIFFV_BEM_SURF_ID_HEAD, &MNEBemSurface::id);
    if (head == bem.end()) {
        head = std::prev(bem.end());
        Log::info("No explicit outer skin surface found, using the outermost surface.");
    }
    const MNEBemSurface& skin = *head;
    Log::info("Using surface {}: {} vertices, {} triangles", head - bem.begin(), skin.np, skin.ntri);

    // Create output directory
    QDir dir;
    if (!dir.mkpath(outDir)) {
        Log::critical("Cannot create output directory: {}", outDir.toStdString());
        return 1;
    }

    // Generate decimated surfaces
    for (int targetVerts : grades) {
        Log::info("Decimating to {} vertices...", targetVerts);
        const std::vector<MNEBemSurface> surfaces =
            MNEBemSurface::makeScalpSurfaces(skin, {targetVerts});
        if (surfaces.empty()) {
            Log::critical("Could not decimate scalp surface to {} vertices.", targetVerts);
            return 1;
        }
        const MNEBemSurface& decimated = surfaces.front();

        // Write as FIFF BEM surface
        QString outPath = QDir(outDir).filePath(
            QString("scalp-%1.fif").arg(targetVerts));

        MNEBem outBem;
        outBem.push_back(decimated);

        if (auto written = outBem.write(outPath.toStdU16String()); !written) {
            Log::critical("{}", written.error().what());
            return 1;
        }

        Log::info("  Written {} vertices, {} triangles to {}", decimated.np, decimated.ntri, outPath.toStdString());
    }

    Log::info("Done.");
    return 0;
}
