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
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

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
    CommandLineParser parser("Modify the n_ave (nave) field in evoked data.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Input evoked FIFF file.", "file", {}});
    parser.add_option({{"nave"}, "New nave value.", "value", {}});
    parser.add_option({{"set"}, "Which evoked set index to modify.", "index", "0"});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_change_nave", PROGRAM_VERSION);

    const std::string measFile = parser.value("meas");
    const std::string outFile = parser.value("out");

    if (measFile.empty()) {
        Log::critical("--meas is required.");
        return 1;
    }
    if (!parser.is_set("nave")) {
        Log::critical("--nave is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    int newNave = to_int(parser.value("nave")).value_or(0);
    int setIdx = to_int(parser.value("set")).value_or(0);

    // Read evoked data
    auto read = FiffEvokedSet::read(path_from_utf8(measFile), {}, {}, false);
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
    if (!evokedSet.save(path_from_utf8(outFile))) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }

    Log::info("Written updated evoked data to: {}", outFile);
    return 0;
}
