//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

    printf("Read %lld evoked data set(s) from %s\n",
           static_cast<long long>(evokedSet.evoked.size()), qPrintable(measFile));
    printf("Applying baseline correction [%g, %g] s\n", bmin, bmax);

    // Apply baseline correction to each evoked dataset
    QPair<float,float> baseline(bmin, bmax);
    for (int i = 0; i < evokedSet.evoked.size(); ++i) {
        evokedSet.evoked[i].applyBaselineCorrection(baseline);
        printf("  Set %d (%s): baseline corrected\n", i, qPrintable(evokedSet.evoked[i].comment));
    }

    // Save
    if (!evokedSet.save(outFile)) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }

    printf("Written baseline-corrected evoked data to: %s\n", qPrintable(outFile));
    return 0;
}
