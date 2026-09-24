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
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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

    if (measFile.isEmpty()) { qCritical("--meas is required."); return 1; }
    if (!parser.isSet(bminOpt)) { qCritical("--bmin is required."); return 1; }
    if (!parser.isSet(bmaxOpt)) { qCritical("--bmax is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    float bmin = parser.value(bminOpt).toFloat();
    float bmax = parser.value(bmaxOpt).toFloat();

    // Read evoked data without baseline correction
    QFile file(measFile);
    FiffEvokedSet evokedSet;
    QPair<float,float> noBl(0.0f, 0.0f);
    if (!FiffEvokedSet::read(file, evokedSet, noBl, false)) {
        qCritical("Cannot read evoked data from: %s", qPrintable(measFile));
        return 1;
    }

    qInfo("Read %lld evoked data set(s) from %s" ,
           static_cast<long long>(evokedSet.evoked.size()), qPrintable(measFile));
    qInfo("Applying baseline correction [%g, %g] s" , bmin, bmax);

    // Apply baseline correction to each evoked dataset
    QPair<float,float> baseline(bmin, bmax);
    for (int i = 0; i < evokedSet.evoked.size(); ++i) {
        evokedSet.evoked[i].applyBaselineCorrection(baseline);
        qInfo("  Set %d (%s): baseline corrected" , i, qPrintable(evokedSet.evoked[i].comment));
    }

    // Save
    if (!evokedSet.save(outFile)) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }

    qInfo("Written baseline-corrected evoked data to: %s" , qPrintable(outFile));
    return 0;
}
