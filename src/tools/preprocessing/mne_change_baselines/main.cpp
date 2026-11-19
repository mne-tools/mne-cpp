//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Change baseline in evoked data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <base/string_utils.h>
#include <base/log.h>
#include <base/log_qt.h>

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
    mne::install_qt_message_handler();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_change_baselines");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Change baseline in evoked data.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Input evoked FIFF file.", "file");
    parser.addOption(measOpt);

    QCommandLineOption bminOpt("bmin", "Baseline start in seconds.", "seconds");
    parser.addOption(bminOpt);

    QCommandLineOption bmaxOpt("bmax", "Baseline end in seconds.", "seconds");
    parser.addOption(bmaxOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString measFile = parser.value(measOpt);
    QString outFile = parser.value(outOpt);

    if (measFile.isEmpty()) {
        qCritical("--meas is required.");
        return 1;
    }
    if (!parser.isSet(bminOpt)) {
        qCritical("--bmin is required.");
        return 1;
    }
    if (!parser.isSet(bmaxOpt)) {
        qCritical("--bmax is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        qCritical("--out is required.");
        return 1;
    }

    float bmin = parser.value(bminOpt).toFloat();
    float bmax = parser.value(bmaxOpt).toFloat();

    // Read evoked data without baseline correction
    auto read = FiffEvokedSet::read(measFile.toStdU16String(), {}, {}, false);
    if (!read) {
        qCritical("%s", read.error().what());
        return 1;
    }
    FiffEvokedSet& evokedSet = *read;

    qInfo("Read %lld evoked data set(s) from %s",
          static_cast<long long>(evokedSet.evoked.size()), qPrintable(measFile));
    qInfo("Applying baseline correction [%g, %g] s", bmin, bmax);

    // Apply baseline correction to each evoked dataset
    QPair<float, float> baseline(bmin, bmax);
    for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
        evokedSet.evoked[i].applyBaselineCorrection(baseline);
        qInfo("  Set %d (%s): baseline corrected", i, evokedSet.evoked[i].comment.c_str());
    }

    // Save
    if (!evokedSet.save(outFile.toStdU16String())) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }

    qInfo("Written baseline-corrected evoked data to: %s", qPrintable(outFile));
    return 0;
}
