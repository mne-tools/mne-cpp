//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2004
 * @brief    Compare two FIFF files tag by tag.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>
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

static std::string tagName(int kind)
{
    // Return a human-readable tag name for common FIFF tags
    switch (kind) {
        case FIFF_FILE_ID:
            return "FILE_ID";
        case FIFF_DIR_POINTER:
            return "DIR_POINTER";
        case FIFF_DIR:
            return "DIR";
        case FIFF_BLOCK_ID:
            return "BLOCK_ID";
        case FIFF_BLOCK_START:
            return "BLOCK_START";
        case FIFF_BLOCK_END:
            return "BLOCK_END";
        case FIFF_FREE_LIST:
            return "FREE_LIST";
        case FIFF_NCHAN:
            return "NCHAN";
        case FIFF_SFREQ:
            return "SFREQ";
        case FIFF_CH_INFO:
            return "CH_INFO";
        case FIFF_MEAS_DATE:
            return "MEAS_DATE";
        case FIFF_COORD_TRANS:
            return "COORD_TRANS";
        case FIFF_NAVE:
            return "NAVE";
        case FIFF_FIRST_SAMPLE:
            return "FIRST_SAMPLE";
        case FIFF_LAST_SAMPLE:
            return "LAST_SAMPLE";
        case FIFF_COMMENT:
            return "COMMENT";
        default:
            return std::format("TAG_{}", kind);
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Compare two FIFF files tag by tag.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"file1"}, "First FIFF file.", "file", {}});
    parser.add_option({{"file2"}, "Second FIFF file.", "file", {}});
    parser.add_option({{"verbose"}, "Show matching tags too.", {}, {}});
    parser.process(argc, argv, "mne_compare_fif_files", PROGRAM_VERSION);

    const std::string file1 = parser.value("file1");
    const std::string file2 = parser.value("file2");
    bool verbose = parser.is_set("verbose");

    if (file1.empty() || file2.empty()) {
        Log::critical("Both --file1 and --file2 are required.");
        return 1;
    }

    // Open both files
    FiffStream stream1(path_from_utf8(file1));
    FiffStream stream2(path_from_utf8(file2));

    if (!stream1.open()) {
        Log::critical("Cannot parse FIFF: {}", file1);
        return 1;
    }

    if (!stream2.open()) {
        Log::critical("Cannot parse FIFF: {}", file2);
        return 1;
    }

    const std::vector<FiffDirEntry>& dir1 = stream1.dir();
    const std::vector<FiffDirEntry>& dir2 = stream2.dir();

    Log::info("File 1: {} ({} directory entries)", file1, static_cast<int>(dir1.size()));
    Log::info("File 2: {} ({} directory entries)", file2, static_cast<int>(dir2.size()));
    Log::info("{}", "");

    int nMatch = 0, nDiff = 0, nMissing = 0;
    int minEntries = static_cast<int>(std::min(dir1.size(), dir2.size()));

    for (int i = 0; i < minEntries; ++i) {
        // The directory ends with an entry that marks its end, not a tag.
        if (dir1[i].pos < 0 && dir2[i].pos < 0) {
            continue;
        }
        const std::optional<FiffTag> tag1 = stream1.read_tag(dir1[i].pos);
        const std::optional<FiffTag> tag2 = stream2.read_tag(dir2[i].pos);

        bool kindMatch = (dir1[i].kind == dir2[i].kind);
        bool typeMatch = (dir1[i].type == dir2[i].type);
        bool sizeMatch = (dir1[i].size == dir2[i].size);

        bool dataMatch = false;
        if (sizeMatch && tag1 && tag2) {
            dataMatch = std::ranges::equal(tag1->bytes(), tag2->bytes());
        }

        bool isMatch = kindMatch && typeMatch && sizeMatch && dataMatch;

        if (isMatch) {
            nMatch++;
            if (verbose)
                Log::info("  [{:4}] {} : MATCH (size={})", i, tagName(dir1[i].kind), dir1[i].size);
        } else {
            nDiff++;
            std::string line = std::format("  [{:4}] DIFF:", i);
            if (!kindMatch)
                line += std::format(" kind={} vs {}", dir1[i].kind, dir2[i].kind);
            else
                line += " kind=" + tagName(dir1[i].kind);
            if (!typeMatch)
                line += std::format(" type={} vs {}", dir1[i].type, dir2[i].type);
            if (!sizeMatch)
                line += std::format(" size={} vs {}", dir1[i].size, dir2[i].size);
            if (sizeMatch && !dataMatch)
                line += " DATA_DIFFERS";
            Log::info("{}", line);
        }
    }

    if (dir1.size() != dir2.size()) {
        nMissing = static_cast<int>(std::abs(std::ssize(dir1) - std::ssize(dir2)));
        Log::info("\n  File {} has {} extra directory entries.", dir1.size() > dir2.size() ? "1" : "2", nMissing);
    }

    Log::info("\nSummary: {} matching, {} different, {} extra entries", nMatch, nDiff, nMissing);

    bool identical = (nDiff == 0 && nMissing == 0);
    Log::info("Files are {}.", identical ? "IDENTICAL" : "DIFFERENT");

    stream1.close();
    stream2.close();

    return identical ? 0 : 1;
}
