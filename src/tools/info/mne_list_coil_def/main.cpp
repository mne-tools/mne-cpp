//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    List all available coil definitions from a coil definition file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fwd/fwd_coil_set.h>
#include <mne/fwd/fwd_coil.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <memory>
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

static const char* coilClassName(int coilClass)
{
    switch (coilClass) {
        case FWD_COILC_MAG:
            return "MAG";
        case FWD_COILC_AXIAL_GRAD:
            return "AXIAL_GRAD";
        case FWD_COILC_PLANAR_GRAD:
            return "PLANAR_GRAD";
        case FWD_COILC_AXIAL_GRAD2:
            return "AXIAL_GRAD2";
        case FWD_COILC_EEG:
            return "EEG";
        default:
            return "UNKNOWN";
    }
}

//=============================================================================================================

static const char* accuracyName(int acc)
{
    switch (acc) {
        case FWD_COIL_ACCURACY_POINT:
            return "point";
        case FWD_COIL_ACCURACY_NORMAL:
            return "normal";
        case FWD_COIL_ACCURACY_ACCURATE:
            return "accurate";
        default:
            return "unknown";
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("List all available coil definitions.\n\nReads a coil definition file and prints a table of coil types.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"coildef"}, "Custom coil definition file (optional).", "name", {}});
    parser.process(argc, argv, "mne_list_coil_def", PROGRAM_VERSION);

    std::string coildefName = parser.value("coildef");

    // If no custom file, use the default coil_def.dat location
    if (coildefName.empty()) {
        // Try standard locations
        const std::string appDir = utf8_from_path(executable_dir());
        const StringList searchPaths = {appDir + "/../resources/coil_def.dat",
                                        appDir + "/resources/coil_def.dat",
                                        appDir + "/coil_def.dat"};

        for (const std::string& path : searchPaths) {
            std::error_code ec;
            if (std::filesystem::exists(path_from_utf8(path), ec)) {
                coildefName = path;
                break;
            }
        }

        if (coildefName.empty()) {
            Log::critical("No coil definition file found. Use --coildef to specify one.");
            return 1;
        }
    }

    Log::info("Reading coil definitions from: {}", coildefName);

    std::unique_ptr<FwdCoilSet> coilSet = FwdCoilSet::read_coil_defs(path_from_utf8(coildefName));
    if (!coilSet) {
        Log::critical("Cannot read coil definitions from: {}", coildefName);
        return 1;
    }

    int ncoils = coilSet->ncoil();
    Log::info("Read {} coil definitions.\n", ncoils);

    // Print header
    Log::info("{:<8} {:<14} {:<10} {:<6} {:<10} {}", "Type", "Class", "Accuracy", "NP", "Size(m)", "Description");
    Log::info("{:<8} {:<14} {:<10} {:<6} {:<10} {}", "--------", "--------------", "----------", "------", "----------", "--------------------");

    for (int k = 0; k < ncoils; ++k) {
        const FwdCoil& coil = *(coilSet->coils[k]);
        Log::info("{:<8} {:<14} {:<10} {:<6} {:<10.4f} {}", coil.type, coilClassName(coil.coil_class), accuracyName(coil.accuracy), coil.np, coil.size, coil.desc);
    }

    return 0;
}
