//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    List SSP projectors from a FIFF measurement file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_named_matrix.h>
#include <mne/fiff/fiff_constants.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <print>
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

static std::string projKindName(int kind)
{
    switch (kind) {
        case 1:
            return "Field";
        case 2:
            return "Fixed dipole";
        case 3:
            return "Rotating dipole";
        case 4:
            return "Homog. grad.";
        case 5:
            return "Homog. field";
        case FIFFV_MNE_PROJ_ITEM_EEG_AVREF:
            return "EEG avg ref";
        default:
            return std::format("Unknown ({})", kind);
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("List SSP projectors from a FIFF file.\n\nReads measurement info and prints projector details.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Measurement FIFF file.", "name", {}});
    parser.process(argc, argv, "mne_list_proj", PROGRAM_VERSION);

    const std::string measName = parser.value("meas");

    if (measName.empty()) {
        Log::critical("The --meas option is required.");
        std::print("{}", parser.help_text("mne_list_proj"));
        return 1;
    }

    // Read measurement info
    FiffRawData raw(path_from_utf8(measName));

    if (raw.info.chs.empty()) {
        Log::critical("No channel information found in {}", measName);
        return 1;
    }

    Log::info("Opened {} : {} channels", measName, raw.info.nchan);

    const std::vector<FiffProj>& projs = raw.info.projs;

    if (projs.empty()) {
        Log::info("No SSP projectors found.");
        return 0;
    }

    Log::info("Found {} SSP projector(s):\n", static_cast<int>(projs.size()));

    // Print header
    Log::info("{:<6} {:<20} {:<8} {:<8} {}", "Index", "Kind", "NVec", "Active", "Description");
    Log::info("{:<6} {:<20} {:<8} {:<8} {}", "------", "--------------------", "--------", "--------", "--------------------");

    for (int k = 0; k < std::ssize(projs); ++k) {
        const FiffProj& proj = projs[k];

        const int nVec = proj.data.empty() ? 0 : proj.data.nrow;

        Log::info("{:<6} {:<20} {:<8} {:<8} {}", k + 1, projKindName(proj.kind), nVec, proj.active ? "Yes" : "No", proj.desc);
    }

    return 0;
}
