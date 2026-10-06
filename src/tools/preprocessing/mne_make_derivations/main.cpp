//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert an MNE-C channel derivation text file into a FIFF derivation file (MNELIB::MNEDerivSet).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_deriv_set.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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
    QCoreApplication::setApplicationName("mne_make_derivations");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Parse text channel derivation file and write as FIFF.\n\n"
                                     "Text format (MNE-C mne_make_derivations):\n"
                                     "  \"derived name\" = coeff1 * \"ch name1\" + coeff2 * \"ch name2\" - \"ch name3\" ...");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input text derivation file.", "file");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString outFile = parser.value(outOpt);

    if (inFile.isEmpty()) {
        qCritical("--in is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        qCritical("--out is required.");
        return 1;
    }

    const std::optional<MNEDerivSet> derivations = MNEDerivSet::readText(inFile);
    if (!derivations) {
        qCritical("No derivations read from: %s", qPrintable(inFile));
        return 1;
    }
    qInfo("Read %d derivation(s) from %s", derivations->count(), qPrintable(inFile));
    if (!derivations->write(outFile)) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }
    qInfo("Written derivations to: %s", qPrintable(outFile));
    return 0;
}
