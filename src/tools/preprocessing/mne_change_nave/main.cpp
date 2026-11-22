//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Modify n_ave field in evoked data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
    QCoreApplication::setApplicationName("mne_change_nave");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Modify the n_ave (nave) field in evoked data.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Input evoked FIFF file.", "file");
    parser.addOption(measOpt);

    QCommandLineOption naveOpt("nave", "New nave value.", "value");
    parser.addOption(naveOpt);

    QCommandLineOption setOpt("set", "Which evoked set index to modify (default 0).", "index", "0");
    parser.addOption(setOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString measFile = parser.value(measOpt);
    QString outFile = parser.value(outOpt);

    if (measFile.isEmpty()) {
        Log::critical("--meas is required.");
        return 1;
    }
    if (!parser.isSet(naveOpt)) {
        Log::critical("--nave is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }

    int newNave = parser.value(naveOpt).toInt();
    int setIdx = parser.value(setOpt).toInt();

    // Read evoked data
    auto read = FiffEvokedSet::read(measFile.toStdU16String(), {}, {}, false);
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    FiffEvokedSet& evokedSet = *read;

    if (setIdx < 0 || setIdx >= std::ssize(evokedSet.evoked)) {
        Log::critical("Set index {} out of range (0..{}).", setIdx, static_cast<long long>(evokedSet.evoked.size() - 1));
        return 1;
    }

    Log::info("Changing nave for set {} ({}): {} -> {}", setIdx, evokedSet.evoked[setIdx].comment, evokedSet.evoked[setIdx].nave, newNave);

    evokedSet.evoked[setIdx].nave = newNave;

    // Save
    if (!evokedSet.save(outFile.toStdU16String())) {
        Log::critical("Cannot write output file: {}", outFile.toStdString());
        return 1;
    }

    Log::info("Written updated evoked data to: {}", outFile.toStdString());
    return 0;
}
