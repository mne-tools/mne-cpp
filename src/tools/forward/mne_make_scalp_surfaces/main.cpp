//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
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
 * @brief    Generate decimated scalp surfaces from BEM outer skin.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_types.h>
#include <utils/generics/mne_logger.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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

    if (bemFile.isEmpty()) { qCritical("--bem is required."); return 1; }
    if (outDir.isEmpty()) { qCritical("--out is required."); return 1; }

    // Parse grades
    QStringList gradesParts = gradesStr.split(',', Qt::SkipEmptyParts);
    QList<int> grades;
    for (const QString &g : gradesParts) {
        int val = g.trimmed().toInt();
        if (val > 0) grades.append(val);
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
    QFile file(bemFile);
    MNEBem bem(file);
    if (bem.size() == 0) {
        qCritical("Cannot read BEM from: %s", qPrintable(bemFile));
        return 1;
    }

    qInfo("Read BEM with %d surface(s) from %s" , bem.size(), qPrintable(bemFile));

    // Find outer skin surface (typically the last/outermost surface, or id == FIFFV_BEM_SURF_ID_HEAD)
    int skinIdx = -1;
    for (int i = 0; i < bem.size(); ++i) {
        if (bem[i].id == FIFFV_BEM_SURF_ID_HEAD) {
            skinIdx = i;
            break;
        }
    }
    // Fallback: use the last surface (outermost)
    if (skinIdx < 0) {
        skinIdx = bem.size() - 1;
        qInfo("No explicit outer skin surface found, using surface %d." , skinIdx);
    }

    const MNEBemSurface &skin = bem[skinIdx];
    qInfo("Using surface %d: %d vertices, %d triangles" , skinIdx, skin.np, skin.ntri);

    // Create output directory
    QDir dir;
    if (!dir.mkpath(outDir)) {
        qCritical("Cannot create output directory: %s", qPrintable(outDir));
        return 1;
    }

    // Generate decimated surfaces
    for (int targetVerts : grades) {
        qInfo("Decimating to %d vertices..." , targetVerts);
        const QList<MNEBemSurface> surfaces =
            MNEBemSurface::makeScalpSurfaces(skin, {targetVerts});
        if (surfaces.isEmpty()) {
            qCritical("Could not decimate scalp surface to %d vertices.", targetVerts);
            return 1;
        }
        const MNEBemSurface& decimated = surfaces.constFirst();

        // Write as FIFF BEM surface
        QString outPath = QDir(outDir).filePath(
            QString("scalp-%1.fif").arg(targetVerts));

        MNEBem outBem;
        outBem << decimated;

        QFile outFile(outPath);
        outBem.write(outFile);

        qInfo("  Written %d vertices, %d triangles to %s" ,
               decimated.np, decimated.ntri, qPrintable(outPath));
    }

    qInfo("Done.");
    return 0;
}
