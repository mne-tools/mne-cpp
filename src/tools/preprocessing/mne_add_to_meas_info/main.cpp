//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2006
 * @brief    Add tags from one FIFF file to the meas_info block of another.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

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

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Add tags from one FIFF file to the meas_info block of another.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"add"}, "FIFF file containing tags to add.", "name", {}});
    parser.add_option({{"dest"}, "Destination FIFF file to receive additional tags.", "name", {}});
    parser.process(argc, argv, "mne_add_to_meas_info", PROGRAM_VERSION);

    const std::string addName = parser.value("add");
    const std::string destName = parser.value("dest");

    if (addName.empty() || destName.empty()) {
        Log::critical("Both --add and --dest are required.");
        std::print("{}", parser.help_text("mne_add_to_meas_info"));
        return 1;
    }

    Log::info("Source file      : {}", addName);
    Log::info("Destination file : {}", destName);

    // Read all tags from the source file
    FiffStream addStream(path_from_utf8(addName));
    if (!addStream.open()) {
        Log::critical("Cannot open source file: {}", addName);
        return 1;
    }

    std::vector<FiffTag> tagsToAdd;
    for (int k = 0; k < addStream.nent(); k++) {
        fiff_int_t kind = addStream.dir()[k].kind;
        // Skip file-level structural tags
        if (kind == FIFF_FILE_ID || kind == FIFF_DIR_POINTER ||
            kind == FIFF_FREE_LIST || kind == FIFF_NOP ||
            kind == FIFF_BLOCK_START || kind == FIFF_BLOCK_END) {
            continue;
        }
        if (auto tag = addStream.read_tag(addStream.dir()[k].pos)) {
            tagsToAdd.push_back(std::move(*tag));
        }
    }
    addStream.close();

    Log::info("{} tags to add", static_cast<int>(tagsToAdd.size()));

    if (tagsToAdd.empty()) {
        Log::info("No tags to add.");
        return 0;
    }

    // Open destination file for update and append tags
    auto destStream = FiffStream::open_update(path_from_utf8(destName));
    if (!destStream) {
        Log::critical("Cannot open destination file for update: {}", destName);
        return 1;
    }

    // Append all tags at the end
    int nAdded = 0;
    for (const FiffTag& tag : tagsToAdd) {
        destStream->write_tag(tag);
        nAdded++;
    }

    destStream->close();

    Log::info("{} tags added to {}", nAdded, destName);
    Log::info("done.");

    return 0;
}
