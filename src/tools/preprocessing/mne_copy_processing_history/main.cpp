//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Copy processing history block between FIFF files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
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
    QCoreApplication::setApplicationName("mne_copy_processing_history");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Copy processing history block from one FIFF file to another.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source FIFF file containing processing history.", "file");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Destination FIFF file (will be modified).", "file");
    parser.addOption(toOpt);

    parser.process(app);

    QString fromFile = parser.value(fromOpt);
    QString toFile = parser.value(toOpt);

    if (fromFile.isEmpty()) {
        qCritical("--from is required.");
        return 1;
    }
    if (toFile.isEmpty()) {
        qCritical("--to is required.");
        return 1;
    }

    if (!FiffStream::copyProcessingHistory(fromFile, toFile)) {
        qCritical("Failed to copy processing history from %s to %s", qPrintable(fromFile), qPrintable(toFile));
        return 1;
    }

    qInfo("Successfully copied processing history from %s to %s",
          qPrintable(fromFile), qPrintable(toFile));
    return 0;
}
