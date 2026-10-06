//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Collect coordinate transformations into one FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_coord_trans_set.h>
#include <fiff/fiff_stream.h>
#include <mri/mri_mgh_io.h>
#include <mri/mri_vol_data.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MRILIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static void printTransform(const FiffCoordTrans& t)
{
    qInfo("%s -> %s transform:",
          qPrintable(FiffCoordTrans::frame_name(t.from)),
          qPrintable(FiffCoordTrans::frame_name(t.to)));
    for (int i = 0; i < 3; i++) {
        qInfo("  %10.6f %10.6f %10.6f  %10.4f mm",
              t.trans(i, 0), t.trans(i, 1), t.trans(i, 2),
              1000.0f * t.trans(i, 3));
    }
    qInfo("%s", "");
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_collect_transforms");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Collect coordinate transforms into one FIFF file.\n\n"
                                     "Reads the device -> head transform of a measurement, the MRI -> head transform "
                                     "of a coregistration, and the surface RAS -> RAS -> MNI Talairach -> Talairach "
                                     "chain of an MRI set or of an mgh/mgz volume with its transforms/talairach.xfm.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "MEG measurement file (device->head transform).", "name");
    QCommandLineOption mriOpt("mri", "FIFF MRI description or -trans.fif file (MRI->head transform and, without --mgh, the Talairach chain).", "name");
    QCommandLineOption mghOpt("mgh", "mgh/mgz MRI volume; its centre and transforms/talairach.xfm give the Talairach chain.", "name");
    QCommandLineOption outOpt("out", "Output file name.", "name");
    parser.addOptions({measOpt, mriOpt, mghOpt, outOpt});
    parser.process(app);

    const QString measName = parser.value(measOpt);
    const QString mriName = parser.value(mriOpt);
    const QString mghName = parser.value(mghOpt);
    const QString outName = parser.value(outOpt);
    if (measName.isEmpty() && mriName.isEmpty() && mghName.isEmpty()) {
        qCritical("At least one of --meas, --mri or --mgh must be specified.");
        parser.showHelp(1);
    }

    FiffCoordTrans devHeadT;
    if (!measName.isEmpty()) {
        devHeadT = FiffCoordTrans::readMeasTransform(measName);
        if (devHeadT.isEmpty()) {
            qCritical("No device->head transform in %s", qPrintable(measName));
            return 1;
        }
    }

    FiffCoordTransSet chain;
    if (!mghName.isEmpty()) {
        MriVolData volume;
        QVector<FiffCoordTrans> volumeTrans;
        const QString mriDir = QFileInfo(mghName).absolutePath();
        if (!MriMghIO::read(mghName, volume, volumeTrans, mriDir)) {
            qCritical("Cannot read %s", qPrintable(mghName));
            return 1;
        }
        for (const FiffCoordTrans& t : volumeTrans) {
            if (t.from == FIFFV_COORD_MRI && t.to == FIFFV_MNE_COORD_RAS) {
                chain.surf_RAS_RAS_t = t;
            }
        }
        // FreeSurfer keeps the Talairach transform next to the volume (MNE-C mne_mri_get_mgh_xform_file_name).
        if (!chain.addTalairach(mriDir + "/transforms/talairach.xfm")) {
            qWarning("No Talairach transform found for %s", qPrintable(mghName));
        }
    }
    if (!mriName.isEmpty()) {
        FiffCoordTransSet fromFile;
        if (fromFile.read(mriName) < 0 || fromFile.head_surf_RAS_t.isEmpty()) {
            qCritical("No MRI->head transform in %s", qPrintable(mriName));
            return 1;
        }
        chain.head_surf_RAS_t = fromFile.head_surf_RAS_t;
        if (mghName.isEmpty()) {
            chain = fromFile;
        }
    }

    if (!devHeadT.isEmpty()) {
        printTransform(devHeadT);
    }
    for (const FiffCoordTrans* t : {&chain.head_surf_RAS_t, &chain.surf_RAS_RAS_t, &chain.RAS_MNI_tal_t, &chain.MNI_tal_tal_gtz_t, &chain.MNI_tal_tal_ltz_t}) {
        if (!t->isEmpty()) {
            printTransform(*t);
        }
    }

    if (!outName.isEmpty()) {
        QFile outFile(outName);
        FiffStream::SPtr outStream = FiffStream::start_file(outFile);
        if (!outStream) {
            qCritical("Cannot open output file: %s", qPrintable(outName));
            return 1;
        }
        if (!devHeadT.isEmpty()) {
            outStream->write_coord_trans(devHeadT);
        }
        chain.write(*outStream);
        outStream->end_file();
        qInfo("Written %s", qPrintable(outName));
    }
    return 0;
}
