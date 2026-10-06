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
 * Writes $SUBJECTS_DIR/morph-maps/<from>-<to>-morph.fif with the maps of both
 * hemispheres in both directions, like MNE-C mne_make_morph_maps and
 * mne.read_morph_map (see MNELIB::MNEMorphMap::compute).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_surface.h>
#include <mne/mne_morph_map.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_morph_maps");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compute the morphing maps between two subjects from their ?h.sphere.reg surfaces.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source subject name.", "subject");
    parser.addOption(fromOpt);
    QCommandLineOption toOpt("to", "Destination subject name.", "subject");
    parser.addOption(toOpt);
    QCommandLineOption subjDirOpt("subjects_dir", "Subjects directory.", "dir", qEnvironmentVariable("SUBJECTS_DIR"));
    parser.addOption(subjDirOpt);
    QCommandLineOption outOpt("out", "Output morph map FIFF file (default $SUBJECTS_DIR/morph-maps/<from>-<to>-morph.fif).", "file");
    parser.addOption(outOpt);

    parser.process(app);

    const QString fromSubject = parser.value(fromOpt);
    const QString toSubject = parser.value(toOpt);
    const QString subjectsDir = parser.value(subjDirOpt);
    QString outFile = parser.value(outOpt);

    if (fromSubject.isEmpty() || toSubject.isEmpty()) {
        qCritical("--from and --to are required.");
        return 1;
    }
    if (subjectsDir.isEmpty()) {
        qCritical("$SUBJECTS_DIR not set.");
        return 1;
    }
    if (outFile.isEmpty()) {
        QDir().mkpath(subjectsDir + "/morph-maps");
        outFile = QString("%1/morph-maps/%2-%3-morph.fif").arg(subjectsDir, fromSubject, toSubject);
    }

    std::vector<MNEMorphMap> maps;
    for (int hemi = 0; hemi < 2; ++hemi) {
        const QString name = QString(hemi == 0 ? "lh" : "rh") + ".sphere.reg";
        FsSurface fromSphere;
        FsSurface toSphere;
        if (!FsSurface::read(QString("%1/%2/surf/%3").arg(subjectsDir, fromSubject, name), fromSphere, false) ||
            !FsSurface::read(QString("%1/%2/surf/%3").arg(subjectsDir, toSubject, name), toSphere, false)) {
            qCritical("Cannot read the %s surfaces.", qPrintable(name));
            return 1;
        }
        MNEMorphMap fromTo = MNEMorphMap::compute(fromSphere.rr(), fromSphere.tris(), toSphere.rr());
        fromTo.hemi = hemi;
        fromTo.from_subj = fromSubject;
        fromTo.to_subj = toSubject;
        MNEMorphMap toFrom = MNEMorphMap::compute(toSphere.rr(), toSphere.tris(), fromSphere.rr());
        toFrom.hemi = hemi;
        toFrom.from_subj = toSubject;
        toFrom.to_subj = fromSubject;
        qInfo("%s: %d -> %d and %d -> %d vertices", qPrintable(name), fromTo.map->cols(), fromTo.map->rows(), toFrom.map->cols(), toFrom.map->rows());
        maps.push_back(std::move(fromTo));
        maps.push_back(std::move(toFrom));
    }
    if (!MNEMorphMap::write(outFile, {&maps[0], &maps[2], &maps[1], &maps[3]})) {
        qCritical("Cannot write %s", qPrintable(outFile));
        return 1;
    }
    qInfo("Written morph maps to: %s", qPrintable(outFile));
    return 0;
}
