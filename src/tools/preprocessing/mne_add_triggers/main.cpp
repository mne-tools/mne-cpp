//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     2001
 * @brief    Add trigger events to raw data's STI channel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_file.h>
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
#include <fstream>
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

struct TriggerEvent
{
    int sample;
    int value;
};

//=============================================================================================================

static std::vector<TriggerEvent> readTriggerFile(const std::string& filename)
{
    std::vector<TriggerEvent> triggers;
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open trigger file: {}", filename);
        return triggers;
    }
    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() >= 2) {
            TriggerEvent evt;
            evt.sample = to_int(parts[0]).value_or(0);
            evt.value = to_int(parts[1]).value_or(0);
            triggers.push_back(evt);
        }
    }
    return triggers;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Add trigger events to raw data's STI channel.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Input raw FIFF file.", "file", {}});
    parser.add_option({{"trg"}, "Trigger file (sample event_value per line).", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_add_triggers", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    const std::string trgFile = parser.value("trg");
    const std::string outFile = parser.value("out");

    if (rawFile.empty()) {
        Log::critical("--raw is required.");
        return 1;
    }
    if (trgFile.empty()) {
        Log::critical("--trg is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Read trigger file
    const std::vector<TriggerEvent> triggers = readTriggerFile(trgFile);
    if (triggers.empty()) {
        Log::critical("No triggers read from: {}", trgFile);
        return 1;
    }
    Log::info("Read {} trigger events from {}", static_cast<long long>(triggers.size()), trgFile);

    // Open raw data
    FiffRawData raw(path_from_utf8(rawFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read raw data from: {}", rawFile);
        return 1;
    }

    // Find STI channel (prefer STI 014, fallback to first stim channel)
    int stiIdx = -1;
    for (int i = 0; i < std::ssize(raw.info.chs); ++i) {
        if ((raw.info.chs[i].ch_name.find("STI") != std::string::npos) ||
            raw.info.chs[i].kind == FIFFV_STIM_CH) {
            if ((raw.info.chs[i].ch_name.find("014") != std::string::npos) || stiIdx < 0) {
                stiIdx = i;
            }
        }
    }

    if (stiIdx < 0) {
        Log::critical("No STI/stimulus channel found in data.");
        return 1;
    }
    Log::info("Using stimulus channel: {} (index {})", raw.info.chs[stiIdx].ch_name, stiIdx);

    // Read all data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        Log::critical("Cannot read raw data segment.");
        return 1;
    }

    // Add triggers
    int nAdded = 0;
    for (const TriggerEvent& evt : triggers) {
        int col = evt.sample - raw.first_samp;
        if (col >= 0 && col < data.cols()) {
            data(stiIdx, col) = static_cast<double>(evt.value);
            ++nAdded;
        } else {
            Log::warning("Trigger sample {} out of range [{}, {}], skipping.", evt.sample, raw.first_samp, raw.last_samp);
        }
    }
    Log::info("Added {} trigger events.", nAdded);

    // Write output
    RowVectorXd cals;
    auto outStream = FiffStream::start_writing_raw(path_from_utf8(outFile), raw.info, cals);
    if (!outStream) {
        Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }

    fiff_int_t firstSample = raw.first_samp;
    outStream->write_int(FIFF_FIRST_SAMPLE, &firstSample);

    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk, cals);
    }
    outStream->finish_writing_raw();

    Log::info("Written modified raw data to: {}", outFile);
    return 0;
}
