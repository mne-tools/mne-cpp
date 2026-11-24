//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     2006
 * @brief    Toggle skip tags in raw data (in-place).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>
#include <mne/fiff/fiff_types.h>
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
    CommandLineParser parser("Toggle skip tags in raw FIFF data.\n\n"
                             "This modifies the file in-place, toggling FIFF_DATA_SKIP tags\n"
                             "between active (skip) and inactive (unskip) states.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Input raw FIFF file (modified in-place).", "file", {}});
    parser.process(argc, argv, "mne_toggle_skips", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    if (rawFile.empty()) {
        Log::critical("--raw is required.");
        return 1;
    }

    // Open file for update
    auto stream = FiffStream::open_update(path_from_utf8(rawFile));
    if (!stream) {
        Log::critical("Cannot open FIFF file for update: {}", rawFile);
        return 1;
    }

    // As MNE-C: a skip is turned off by renaming it FIFF_MNE_DATA_SKIP_NOP, which readers ignore, and on again
    int turnedOff = 0;
    int turnedOn = 0;
    for (const FiffDirEntry& entry : stream->dir()) {
        if (entry.kind != FIFF_DATA_SKIP && entry.kind != FIFF_MNE_DATA_SKIP_NOP) {
            continue;
        }
        std::optional<FiffTag> tag = stream->read_tag(entry.pos);
        if (!tag) {
            Log::critical("Cannot read the tag at position {}.", static_cast<long long>(entry.pos));
            return 1;
        }
        const bool on = tag->kind == FIFF_DATA_SKIP;
        tag->kind = on ? FIFF_MNE_DATA_SKIP_NOP : FIFF_DATA_SKIP;
        stream->write_tag(*tag, entry.pos);
        ++(on ? turnedOff : turnedOn);
    }
    stream->close();

    Log::info("{} skips turned off, {} skips turned on in {}", turnedOff, turnedOn, rawFile);
    return 0;
}
