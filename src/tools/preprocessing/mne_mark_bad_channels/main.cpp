//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2001
 * @brief    Mark or update bad channels in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_types.h>
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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static bool isReadable(const std::string& filename)
{
    return std::ifstream(path_from_utf8(filename)).is_open();
}

//=============================================================================================================

static StringList readBadChannelList(const std::string& filename)
{
    StringList bads;
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open bad channel file: {}", filename);
        return bads;
    }
    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (!line.empty() && !line.starts_with('#'))
            bads.emplace_back(line);
    }
    return bads;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Mark or update bad channels in a FIFF file.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"bad"}, "Text file listing bad channel names (one per line).", "file", {}});
    parser.add_option({{"fif"}, "FIFF file to modify.", "file", {}});
    parser.process(argc, argv, "mne_mark_bad_channels", PROGRAM_VERSION);

    const std::string badName = parser.value("bad");
    const std::string fifName = parser.value("fif");

    if (fifName.empty() || badName.empty()) {
        Log::critical("Both --fif and --bad are required.");
        return 1;
    }
    if (!isReadable(badName)) {
        Log::critical("Cannot open bad channel file: {}", badName);
        return 1;
    }
    if (!isReadable(fifName)) {
        Log::critical("Cannot read FIFF file: {}", fifName);
        return 1;
    }

    // Read the bad channel list
    StringList bads = readBadChannelList(badName);
    if (bads.empty()) {
        Log::warning("Bad channel list is empty.");
    }

    Log::info("Bad channels to mark ({}):", static_cast<long long>(bads.size()));
    for (const std::string& ch : bads) {
        Log::info("  {}", ch);
    }

    // Open the FIFF file and read raw data
    FiffRawData raw;
    try {
        raw = FiffRawData(path_from_utf8(fifName));
    } catch (const std::exception& error) {
        Log::critical("Cannot read FIFF file {}: {}", fifName, error.what());
        return 1;
    }
    if (raw.info.empty()) {
        Log::critical("Cannot read FIFF file: {}", fifName);
        return 1;
    }

    // Update bad channels
    Log::info("Previous bad channels: {}", static_cast<long long>(raw.info.bads.size()));
    raw.info.bads = bads;
    Log::info("New bad channels: {}", static_cast<long long>(raw.info.bads.size()));

    // Read all data
    Eigen::MatrixXd data;
    Eigen::MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        Log::critical("Cannot read raw data segment.");
        return 1;
    }

    // Write to temp file, then replace
    const std::string tmpName = fifName + ".tmp";
    Eigen::RowVectorXd cals;
    auto outStream = FiffStream::start_writing_raw(path_from_utf8(tmpName), raw.info, cals);
    if (!outStream) {
        Log::critical("Cannot open temp file for writing.");
        return 1;
    }

    // Write data in chunks
    int nSamples = static_cast<int>(data.cols());
    int nChan = static_cast<int>(data.rows());
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        Eigen::MatrixXd chunk = data.block(0, start, nChan, end - start);
        outStream->write_raw_buffer(chunk);
    }

    outStream->finish_writing_raw();

    // Replace original with temp
    std::error_code error;
    std::filesystem::remove(path_from_utf8(fifName), error);
    std::filesystem::rename(path_from_utf8(tmpName), path_from_utf8(fifName), error);

    Log::info("Successfully updated bad channels in {}", fifName);

    return 0;
}
