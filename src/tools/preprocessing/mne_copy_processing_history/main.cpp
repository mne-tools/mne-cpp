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

#include <mne/fiff/fiff_stream.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
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
        Log::critical("--from is required.");
        return 1;
    }
    if (toFile.isEmpty()) {
        Log::critical("--to is required.");
        return 1;
    }

    if (!FiffStream::copy_processing_history(fromFile.toStdU16String(), toFile.toStdU16String())) {
        Log::critical("Failed to copy processing history from {} to {}", fromFile.toStdString(), toFile.toStdString());
        return 1;
    }

    Log::info("Successfully copied processing history from {} to {}", fromFile.toStdString(), toFile.toStdString());
    return 0;
}
