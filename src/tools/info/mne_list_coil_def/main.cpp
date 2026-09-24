//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    List all available coil definitions from a coil definition file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString coilClassName(int coilClass)
{
    switch (coilClass) {
    case FWD_COILC_MAG:         return "MAG";
    case FWD_COILC_AXIAL_GRAD:  return "AXIAL_GRAD";
    case FWD_COILC_PLANAR_GRAD: return "PLANAR_GRAD";
    case FWD_COILC_AXIAL_GRAD2: return "AXIAL_GRAD2";
    case FWD_COILC_EEG:         return "EEG";
    default:                    return "UNKNOWN";
    }
}

//=============================================================================================================

static QString accuracyName(int acc)
{
    switch (acc) {
    case FWD_COIL_ACCURACY_POINT:    return "point";
    case FWD_COIL_ACCURACY_NORMAL:   return "normal";
    case FWD_COIL_ACCURACY_ACCURATE: return "accurate";
    default:                         return "unknown";
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_coil_def");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("List all available coil definitions.\n\nReads a coil definition file and prints a table of coil types.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption coildefOpt("coildef", "Custom coil definition file (optional).", "name");
    parser.addOption(coildefOpt);

    parser.process(app);

    QString coildefName = parser.value(coildefOpt);

    // If no custom file, use the default coil_def.dat location
    if (coildefName.isEmpty()) {
        // Try standard locations
        QStringList searchPaths;
        searchPaths << QCoreApplication::applicationDirPath() + "/../resources/coil_def.dat"
                    << QCoreApplication::applicationDirPath() + "/resources/coil_def.dat"
                    << QCoreApplication::applicationDirPath() + "/coil_def.dat";

        for (const QString& path : searchPaths) {
            if (QFile::exists(path)) {
                coildefName = path;
                break;
            }
        }

        if (coildefName.isEmpty()) {
            qCritical("No coil definition file found. Use --coildef to specify one.");
            return 1;
        }
    }

    fprintf(stderr, "Reading coil definitions from: %s\n", qPrintable(coildefName));

    FwdCoilSet::UPtr coilSet = FwdCoilSet::read_coil_defs(coildefName);
    if (!coilSet) {
        qCritical("Cannot read coil definitions from: %s", qPrintable(coildefName));
        return 1;
    }

    int ncoils = coilSet->ncoil();
    fprintf(stderr, "Read %d coil definitions.\n\n", ncoils);

    // Print header
    fprintf(stdout, "%-8s %-14s %-10s %-6s %-10s %s\n",
            "Type", "Class", "Accuracy", "NP", "Size(m)", "Description");
    fprintf(stdout, "%-8s %-14s %-10s %-6s %-10s %s\n",
            "--------", "--------------", "----------", "------", "----------", "--------------------");

    for (int k = 0; k < ncoils; ++k) {
        const FwdCoil& coil = *(coilSet->coils[k]);
        fprintf(stdout, "%-8d %-14s %-10s %-6d %-10.4f %s\n",
                coil.type,
                qPrintable(coilClassName(coil.coil_class)),
                qPrintable(accuracyName(coil.accuracy)),
                coil.np,
                coil.size,
                qPrintable(coil.desc));
    }

    return 0;
}
