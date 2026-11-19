//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    List SSP projectors from a FIFF measurement file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_constants.h>

#include <base/log.h>
#include <base/log_qt.h>
#include <base/string_utils.h>

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

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString projKindName(int kind)
{
    switch (kind) {
        case 1:
            return "Field";
        case 2:
            return "Fixed dipole";
        case 3:
            return "Rotating dipole";
        case 4:
            return "Homog. grad.";
        case 5:
            return "Homog. field";
        case FIFFV_MNE_PROJ_ITEM_EEG_AVREF:
            return "EEG avg ref";
        default:
            return QString("Unknown (%1)").arg(kind);
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    mne::install_qt_message_handler();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_proj");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("List SSP projectors from a FIFF file.\n\nReads measurement info and prints projector details.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Measurement FIFF file.", "name");
    parser.addOption(measOpt);

    parser.process(app);

    QString measName = parser.value(measOpt);

    if (measName.isEmpty()) {
        qCritical("The --meas option is required.");
        parser.showHelp(1);
    }

    // Read measurement info
    FiffRawData raw(measName.toStdU16String());

    if (raw.info.chs.empty()) {
        qCritical("No channel information found in %s", qPrintable(measName));
        return 1;
    }

    qInfo("Opened %s : %d channels", qPrintable(measName), raw.info.nchan);

    const std::vector<FiffProj>& projs = raw.info.projs;

    if (projs.empty()) {
        qInfo("No SSP projectors found.");
        return 0;
    }

    qInfo("Found %d SSP projector(s):\n", static_cast<int>(projs.size()));

    // Print header
    qInfo("%-6s %-20s %-8s %-8s %s",
          "Index", "Kind", "NVec", "Active", "Description");
    qInfo("%-6s %-20s %-8s %-8s %s",
          "------", "--------------------", "--------", "--------", "--------------------");

    for (int k = 0; k < std::ssize(projs); ++k) {
        const FiffProj& proj = projs[k];

        const int nVec = proj.data.empty() ? 0 : proj.data.nrow;

        qInfo("%-6d %-20s %-8d %-8s %s",
              k + 1,
              qPrintable(projKindName(proj.kind)),
              nVec,
              proj.active ? "Yes" : "No",
              proj.desc.c_str());
    }

    return 0;
}
