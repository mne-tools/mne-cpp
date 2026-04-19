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
 * @brief    Print human-readable summary of evoked data sets from a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString channelKindName(fiff_int_t kind)
{
    switch (kind) {
    case FIFFV_MEG_CH:      return "MEG";
    case FIFFV_REF_MEG_CH:  return "REF_MEG";
    case FIFFV_EEG_CH:      return "EEG";
    case FIFFV_STIM_CH:     return "STIM";
    case FIFFV_EOG_CH:      return "EOG";
    case FIFFV_ECG_CH:      return "ECG";
    default:                return QString("OTHER(%1)").arg(kind);
    }
}

//=============================================================================================================

static QString coilTypeName(fiff_int_t kind, fiff_int_t coilType)
{
    if (kind != FIFFV_MEG_CH)
        return channelKindName(kind);

    // Distinguish gradiometers from magnetometers by coil type
    // Coil types 2, 3012, 3013, 3024 are planar gradiometers
    // Coil types 3022, 3023, 3024 are magnetometers
    // Simple heuristic: unit T/m -> grad, T -> mag
    switch (coilType) {
    case 2:
    case 3012:
    case 3013:
    case 3014:
        return "MEG_GRAD";
    case 3022:
    case 3023:
    case 3024:
        return "MEG_MAG";
    default:
        return "MEG";
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_evoked_data_summary");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Print human-readable summary of evoked data.\n\n"
                                     "Reads all evoked data sets from a FIFF file and prints\n"
                                     "set number, comment, nave, time range, channel count,\n"
                                     "and channel type breakdown.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Evoked FIFF file.", "file");
    parser.addOption(measOpt);

    parser.process(app);

    QString measFile = parser.value(measOpt);
    if (measFile.isEmpty()) {
        qCritical("--meas is required.");
        parser.showHelp(1);
    }

    // Read evoked data
    QFile fMeas(measFile);
    FiffEvokedSet evokedSet;
    if (!FiffEvokedSet::read(fMeas, evokedSet, defaultFloatPair, false)) {
        qCritical("Cannot read evoked data from: %s", qPrintable(measFile));
        return 1;
    }

    printf("File: %s\n", qPrintable(measFile));
    printf("Number of evoked data sets: %lld\n\n", static_cast<long long>(evokedSet.evoked.size()));

    for (int i = 0; i < evokedSet.evoked.size(); ++i) {
        const FiffEvoked &evoked = evokedSet.evoked[i];

        printf("--- Set %d ---\n", i + 1);
        printf("  Comment    : %s\n", qPrintable(evoked.comment));
        printf("  Type       : %s\n", qPrintable(evoked.aspectKindToString()));
        printf("  Nave       : %d\n", evoked.nave);

        float tmin = evoked.times.size() > 0 ? evoked.times(0) : 0.0f;
        float tmax = evoked.times.size() > 0 ? evoked.times(evoked.times.size() - 1) : 0.0f;
        printf("  Time range : %.3f to %.3f s\n", tmin, tmax);
        printf("  Channels   : %d\n", evoked.info.nchan);

        // Count channel types
        QMap<QString, int> typeCounts;
        for (int c = 0; c < evoked.info.chs.size(); ++c) {
            const FiffChInfo &ch = evoked.info.chs[c];
            QString typeName = coilTypeName(ch.kind, ch.chpos.coil_type);
            typeCounts[typeName]++;
        }

        printf("  Channel types:\n");
        for (auto it = typeCounts.constBegin(); it != typeCounts.constEnd(); ++it) {
            printf("    %-12s : %d\n", qPrintable(it.key()), it.value());
        }
        printf("\n");
    }

    return 0;
}
