//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert an MNE-C channel derivation text file into a FIFF derivation file (mne::MNEDerivSet).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_deriv_set.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
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
    CommandLineParser parser("Parse text channel derivation file and write as FIFF.\n\n"
                             "Text format (MNE-C mne_make_derivations):\n"
                             "  \"derived name\" = coeff1 * \"ch name1\" + coeff2 * \"ch name2\" - \"ch name3\" ...");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"in"}, "Input text derivation file.", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_make_derivations", PROGRAM_VERSION);

    const std::string inFile = parser.value("in");
    const std::string outFile = parser.value("out");

    if (inFile.empty()) {
        Log::critical("--in is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    const std::optional<MNEDerivSet> derivations = MNEDerivSet::readText(path_from_utf8(inFile));
    if (!derivations) {
        Log::critical("No derivations read from: {}", inFile);
        return 1;
    }
    Log::info("Read {} derivation(s) from {}", derivations->count(), inFile);
    if (!derivations->write(path_from_utf8(outFile))) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }
    Log::info("Written derivations to: {}", outFile);
    return 0;
}
