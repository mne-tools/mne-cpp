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
 * @brief    Modify n_ave field in evoked data.
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

    if (measFile.isEmpty()) { qCritical("--meas is required."); return 1; }
    if (!parser.isSet(naveOpt)) { qCritical("--nave is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    int newNave = parser.value(naveOpt).toInt();
    int setIdx = parser.value(setOpt).toInt();

    // Read evoked data
    QFile file(measFile);
    FiffEvokedSet evokedSet;
    if (!FiffEvokedSet::read(file, evokedSet, QPair<float,float>(0.0f, 0.0f), false)) {
        qCritical("Cannot read evoked data from: %s", qPrintable(measFile));
        return 1;
    }

    if (setIdx < 0 || setIdx >= evokedSet.evoked.size()) {
        qCritical("Set index %d out of range (0..%lld).", setIdx,
                  static_cast<long long>(evokedSet.evoked.size() - 1));
        return 1;
    }

    printf("Changing nave for set %d (%s): %d -> %d\n",
           setIdx, qPrintable(evokedSet.evoked[setIdx].comment),
           evokedSet.evoked[setIdx].nave, newNave);

    evokedSet.evoked[setIdx].nave = newNave;

    // Save
    if (!evokedSet.save(outFile)) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        return 1;
    }

    printf("Written updated evoked data to: %s\n", qPrintable(outFile));
    return 0;
}
