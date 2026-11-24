//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     2005
 * @brief    Validate EEG electrode positions in a FIFF measurement file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <print>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Validate EEG electrode positions in a FIFF file.\n\nChecks for undefined, duplicate, and out-of-range electrode locations.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Measurement FIFF file.", "name", {}});
    parser.process(argc, argv, "mne_check_eeg_locations", PROGRAM_VERSION);

    const std::string measName = parser.value("meas");

    if (measName.empty()) {
        Log::critical("The --meas option is required.");
        std::print("{}", parser.help_text("mne_check_eeg_locations"));
        return 1;
    }

    // Read measurement info
    FiffRawData raw(path_from_utf8(measName));

    if (raw.info.chs.empty()) {
        Log::critical("No channel information found in {}", measName);
        return 1;
    }

    Log::info("Opened {} : {} channels", measName, raw.info.nchan);

    // Collect EEG channels
    struct EegEntry
    {
        int index;
        std::string name;
        Vector3f pos;
    };

    std::vector<EegEntry> eegChannels;
    for (int k = 0; k < raw.info.nchan; ++k) {
        const FiffChInfo& ch = raw.info.chs[k];
        if (ch.kind == FIFFV_EEG_CH) {
            EegEntry e;
            e.index = k;
            e.name = ch.ch_name;
            e.pos = ch.chpos.r0;
            eegChannels.push_back(e);
        }
    }

    if (eegChannels.empty()) {
        Log::info("No EEG channels found.");
        return 0;
    }

    Log::info("Found {} EEG channels.", static_cast<int>(eegChannels.size()));

    int nProblems = 0;

    // Check 1: Undefined locations (all zeros)
    for (const auto& e : eegChannels) {
        if (e.pos.norm() < 1e-10f) {
            Log::warning("WARNING: {} (ch {}): location is undefined (all zeros).", e.name, e.index + 1);
            ++nProblems;
        }
    }

    // Check 2: Duplicate positions (within 1mm tolerance)
    constexpr float dupTol = 0.001f; // 1 mm
    for (int i = 0; i < std::ssize(eegChannels); ++i) {
        if (eegChannels[i].pos.norm() < 1e-10f)
            continue; // skip undefined
        for (int j = i + 1; j < std::ssize(eegChannels); ++j) {
            if (eegChannels[j].pos.norm() < 1e-10f)
                continue;
            float dist = (eegChannels[i].pos - eegChannels[j].pos).norm();
            if (dist < dupTol) {
                Log::warning("WARNING: {} and {} have duplicate positions (distance = {:.4f} mm).", eegChannels[i].name, eegChannels[j].name, dist * 1000.0f);
                ++nProblems;
            }
        }
    }

    // Check 3: Distance from origin (typical head radius 0.05 - 0.15 m)
    constexpr float minRadius = 0.05f; // 5 cm
    constexpr float maxRadius = 0.15f; // 15 cm
    for (const auto& e : eegChannels) {
        if (e.pos.norm() < 1e-10f)
            continue; // skip undefined
        float r = e.pos.norm();
        if (r < minRadius || r > maxRadius) {
            Log::warning("WARNING: {} (ch {}): distance from origin = {:.1f} mm (expected {:.0f}-{:.0f} mm).", e.name, e.index + 1, r * 1000.0f, minRadius * 1000.0f, maxRadius * 1000.0f);
            ++nProblems;
        }
    }

    // Summary
    if (nProblems == 0) {
        Log::info("All {} EEG channels have valid locations.", static_cast<int>(eegChannels.size()));
        return 0;
    } else {
        Log::info("\n{} problem(s) found in {} EEG channels.", nProblems, static_cast<int>(eegChannels.size()));
        return 1;
    }
}
