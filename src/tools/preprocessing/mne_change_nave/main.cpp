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

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <base/string_utils.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

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
    mne::Log::install_qt_message_handler();
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
        qCritical("--meas is required.");
        return 1;
    }
    if (!parser.isSet(naveOpt)) {
        qCritical("--nave is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        qCritical("--out is required.");
        return 1;
    }

    int newNave = parser.value(naveOpt).toInt();
    int setIdx = parser.value(setOpt).toInt();

    // Read evoked data
    auto read = FiffEvokedSet::read(measFile.toStdU16String(), {}, {}, false);
    if (!read) {
        qCritical("%s", read.error().what());
        return 1;
    }
    FiffEvokedSet& evokedSet = *read;

    if (setIdx < 0 || setIdx >= std::ssize(evokedSet.evoked)) {
        qCritical("Set index %d out of range (0..%lld).", setIdx,
                  static_cast<long long>(evokedSet.evoked.size() - 1));
        return 1;
    }

    qInfo("Changing nave for set %d (%s): %d -> %d",
          setIdx, evokedSet.evoked[setIdx].comment.c_str(),
          evokedSet.evoked[setIdx].nave, newNave);

    evokedSet.evoked[setIdx].nave = newNave;

    // Save
    if (!evokedSet.save(outFile.toStdU16String())) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }

    qInfo("Written updated evoked data to: %s", qPrintable(outFile));
    return 0;
}
