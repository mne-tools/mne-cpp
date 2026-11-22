//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Print human-readable summary of evoked data sets from a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/string_utils.h>

#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QMap>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString channelKindName(fiff_int_t kind)
{
    switch (kind) {
        case FIFFV_MEG_CH:
            return "MEG";
        case FIFFV_REF_MEG_CH:
            return "REF_MEG";
        case FIFFV_EEG_CH:
            return "EEG";
        case FIFFV_STIM_CH:
            return "STIM";
        case FIFFV_EOG_CH:
            return "EOG";
        case FIFFV_ECG_CH:
            return "ECG";
        default:
            return QString("OTHER(%1)").arg(kind);
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

int main(int argc, char* argv[])
{
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
        Log::critical("--meas is required.");
        parser.showHelp(1);
    }

    // Read evoked data
    auto read = FiffEvokedSet::read(measFile.toStdU16String(), {}, {}, false);
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    FiffEvokedSet& evokedSet = *read;

    Log::info("File: {}", measFile.toStdString());
    Log::info("Number of evoked data sets: {}\n", static_cast<long long>(evokedSet.evoked.size()));

    for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
        const FiffEvoked& evoked = evokedSet.evoked[i];

        Log::info("--- Set {} ---", i + 1);
        Log::info("  Comment    : {}", evoked.comment);
        Log::info("  Type       : {}", evoked.aspectKindToString());
        Log::info("  Nave       : {}", evoked.nave);

        float tmin = evoked.times.size() > 0 ? evoked.times(0) : 0.0f;
        float tmax = evoked.times.size() > 0 ? evoked.times(evoked.times.size() - 1) : 0.0f;
        Log::info("  Time range : {:.3f} to {:.3f} s", tmin, tmax);
        Log::info("  Channels   : {}", evoked.info.nchan);

        // Count channel types
        QMap<QString, int> typeCounts;
        for (int c = 0; c < std::ssize(evoked.info.chs); ++c) {
            const FiffChInfo& ch = evoked.info.chs[c];
            QString typeName = coilTypeName(ch.kind, ch.chpos.coil_type);
            typeCounts[typeName]++;
        }

        Log::info("  Channel types:");
        for (auto it = typeCounts.constBegin(); it != typeCounts.constEnd(); ++it) {
            Log::info("    {:<12} : {}", it.key().toStdString(), it.value());
        }
        Log::info("{}", "");
    }

    return 0;
}
