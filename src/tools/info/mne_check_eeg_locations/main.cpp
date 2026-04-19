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
 * @brief    Validate EEG electrode positions in a FIFF measurement file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_ch_info.h>
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
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_check_eeg_locations");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Validate EEG electrode positions in a FIFF file.\n\nChecks for undefined, duplicate, and out-of-range electrode locations.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Measurement FIFF file.", "name");
    parser.addOption(measOpt);

    parser.process(app);

    QString measName = parser.value(measOpt);

    if (measName.isEmpty()) {
        qCritical("The --meas option is required.");
        parser.showHelp(1);
    }

    // Read measurement info
    QFile measFile(measName);
    FiffRawData raw(measFile);

    if (raw.info.chs.isEmpty()) {
        qCritical("No channel information found in %s", qPrintable(measName));
        return 1;
    }

    fprintf(stderr, "Opened %s : %d channels\n", qPrintable(measName), raw.info.nchan);

    // Collect EEG channels
    struct EegEntry {
        int index;
        QString name;
        Vector3f pos;
    };

    QList<EegEntry> eegChannels;
    for (int k = 0; k < raw.info.nchan; ++k) {
        const FiffChInfo& ch = raw.info.chs[k];
        if (ch.kind == FIFFV_EEG_CH) {
            EegEntry e;
            e.index = k;
            e.name = ch.ch_name;
            e.pos = ch.chpos.r0;
            eegChannels.append(e);
        }
    }

    if (eegChannels.isEmpty()) {
        fprintf(stderr, "No EEG channels found.\n");
        return 0;
    }

    fprintf(stderr, "Found %d EEG channels.\n", eegChannels.size());

    int nProblems = 0;

    // Check 1: Undefined locations (all zeros)
    for (const auto& e : eegChannels) {
        if (e.pos.norm() < 1e-10f) {
            fprintf(stdout, "WARNING: %s (ch %d): location is undefined (all zeros).\n",
                    qPrintable(e.name), e.index + 1);
            ++nProblems;
        }
    }

    // Check 2: Duplicate positions (within 1mm tolerance)
    constexpr float dupTol = 0.001f; // 1 mm
    for (int i = 0; i < eegChannels.size(); ++i) {
        if (eegChannels[i].pos.norm() < 1e-10f)
            continue; // skip undefined
        for (int j = i + 1; j < eegChannels.size(); ++j) {
            if (eegChannels[j].pos.norm() < 1e-10f)
                continue;
            float dist = (eegChannels[i].pos - eegChannels[j].pos).norm();
            if (dist < dupTol) {
                fprintf(stdout, "WARNING: %s and %s have duplicate positions (distance = %.4f mm).\n",
                        qPrintable(eegChannels[i].name),
                        qPrintable(eegChannels[j].name),
                        dist * 1000.0f);
                ++nProblems;
            }
        }
    }

    // Check 3: Distance from origin (typical head radius 0.05 - 0.15 m)
    constexpr float minRadius = 0.05f;  // 5 cm
    constexpr float maxRadius = 0.15f;  // 15 cm
    for (const auto& e : eegChannels) {
        if (e.pos.norm() < 1e-10f)
            continue; // skip undefined
        float r = e.pos.norm();
        if (r < minRadius || r > maxRadius) {
            fprintf(stdout, "WARNING: %s (ch %d): distance from origin = %.1f mm (expected %.0f-%.0f mm).\n",
                    qPrintable(e.name), e.index + 1,
                    r * 1000.0f, minRadius * 1000.0f, maxRadius * 1000.0f);
            ++nProblems;
        }
    }

    // Summary
    if (nProblems == 0) {
        fprintf(stdout, "All %d EEG channels have valid locations.\n", eegChannels.size());
        return 0;
    } else {
        fprintf(stdout, "\n%d problem(s) found in %d EEG channels.\n", nProblems, eegChannels.size());
        return 1;
    }
}
