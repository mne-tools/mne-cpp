//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert an MNE-C channel derivation text file into a FIFF derivation file (mne::MNEDerivSet).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_deriv_set.h>
#include <base/log.h>

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
        Log::critical("--in is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }

    const std::optional<MNEDerivSet> derivations = MNEDerivSet::readText(inFile.toStdU16String());
    if (!derivations) {
        Log::critical("No derivations read from: {}", inFile.toStdString());
        return 1;
    }
    Log::info("Read {} derivation(s) from {}", derivations->count(), inFile.toStdString());
    if (!derivations->write(outFile.toStdU16String())) {
        Log::critical("Cannot write output file: {}", outFile.toStdString());
        return 1;
    }
    Log::info("Written derivations to: {}", outFile.toStdString());
    return 0;
}
