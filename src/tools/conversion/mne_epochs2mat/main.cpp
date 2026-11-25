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
 * @brief    Export epochs from a raw FIFF file to MATLAB Level 5 MAT files.
 *
 *           Reads events from a text file, extracts epochs around matching events,
 *           and writes each epoch as a separate MAT file containing the data matrix,
 *           sampling frequency, and time vector.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/math/mat_file.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
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

struct EventEntry
{
    int sample;
    int prev;
    int id;
};

static std::vector<EventEntry> readEvents(const std::string& filename)
{
    std::vector<EventEntry> events;
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open event file: {}", filename);
        return events;
    }

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() >= 3) {
            EventEntry ev;
            ev.sample = mne::to_int(parts[0]).value_or(0);
            ev.prev = mne::to_int(parts[1]).value_or(0);
            ev.id = mne::to_int(parts[2]).value_or(0);
            events.push_back(ev);
        }
    }
    return events;
}

//=============================================================================================================

/** @return @p text as a float; 0 if it is not a number or out of float range. */
static float epochsToFloat(std::string_view text)
{
    const std::optional<double> value = mne::to_double(text);
    if (!value)
        return 0.0f;
    if (std::isinf(*value) || std::isnan(*value))
        return static_cast<float>(*value);
    if (std::abs(*value) > static_cast<double>(std::numeric_limits<float>::max()))
        return 0.0f;
    return static_cast<float>(*value);
}

//=============================================================================================================

/** @return @p value rounded half away from zero and truncated to int. */
static int epochsRound(double value)
{
    return static_cast<int>(value >= 0.0 ? value + 0.5 : value - 0.5);
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Export epochs to MATLAB .mat format.\n\n"
                             "Reads events from a text file, extracts epochs from raw data\n"
                             "around matching events, and writes each epoch as a MAT5 file.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Raw FIFF file.", "file", {}});
    parser.add_option({{"event"}, "Event file (text: sample prev event_id).", "file", {}});
    parser.add_option({{"tmin"}, "Epoch start time in seconds (e.g. -0.2).", "sec", {}});
    parser.add_option({{"tmax"}, "Epoch end time in seconds (e.g. 0.5).", "sec", {}});
    parser.add_option({{"event-id"}, "Event ID to extract.", "id", {}});
    parser.add_option({{"out"}, "Output directory for .mat files.", "dir", {}});
    parser.process(argc, argv, "mne_epochs2mat", PROGRAM_VERSION);

    const std::string rawFile = parser.value("raw");
    const std::string eventFile = parser.value("event");
    const std::string outDir = parser.value("out");
    float tmin = epochsToFloat(parser.value("tmin"));
    float tmax = epochsToFloat(parser.value("tmax"));
    int eventId = mne::to_int(parser.value("event-id")).value_or(0);

    if (rawFile.empty() || eventFile.empty() || outDir.empty() || !parser.is_set("tmin") || !parser.is_set("tmax") || !parser.is_set("event-id")) {
        Log::critical("--raw, --event, --tmin, --tmax, --event-id, and --out are required.");
        std::print("{}", parser.help_text("mne_epochs2mat"));
        return 1;
    }

    if (tmin >= tmax) {
        Log::critical("--tmin must be less than --tmax.");
        return 1;
    }

    std::error_code fileError;
    if (!std::filesystem::is_regular_file(mne::path_from_utf8(rawFile), fileError) || !std::ifstream(mne::path_from_utf8(rawFile))) {
        Log::critical("Cannot read raw data: {}", rawFile);
        return 1;
    }

    // Create output directory
    const std::filesystem::path dir = mne::path_from_utf8(outDir);
    std::error_code dirError;
    if (!std::filesystem::is_directory(dir, dirError)) {
        std::filesystem::create_directories(dir, dirError);
        if (!std::filesystem::is_directory(dir, dirError)) {
            Log::critical("Cannot create output directory: {}", outDir);
            return 1;
        }
    }

    // Load raw data
    Log::info("Loading raw: {}", rawFile);
    FiffRawData raw(mne::path_from_utf8(rawFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read raw data: {}", rawFile);
        return 1;
    }

    double sfreq = raw.info.sfreq;
    int nChan = raw.info.nchan;
    Log::info("  {} channels, {:.1f} Hz", nChan, sfreq);

    // Read events
    std::vector<EventEntry> events = readEvents(eventFile);
    if (events.empty()) {
        Log::critical("No events read from: {}", eventFile);
        return 1;
    }
    Log::info("Read {} events", static_cast<long long>(events.size()));

    // Compute sample offsets for epoch
    int sminOffset = epochsRound(tmin * sfreq);
    int smaxOffset = epochsRound(tmax * sfreq);
    int epochLen = smaxOffset - sminOffset + 1;

    Log::info("Epoch: {:.3f} to {:.3f} s ({} samples), event ID = {}", tmin, tmax, epochLen, eventId);

    // Build time vector
    MatrixXd timesMat(1, epochLen);
    for (int i = 0; i < epochLen; ++i)
        timesMat(0, i) = (sminOffset + i) / sfreq;

    // Extract and write epochs
    int epochCount = 0;
    for (std::size_t e = 0; e < events.size(); ++e) {
        if (events[e].id != eventId)
            continue;

        int eventSample = events[e].sample;
        int fromSamp = eventSample + sminOffset;
        int toSamp = eventSample + smaxOffset;

        // Check bounds
        if (fromSamp < raw.first_samp || toSamp > raw.last_samp) {
            Log::info("  Skipping event at sample {} (out of bounds)", eventSample);
            continue;
        }

        MatrixXd data;
        MatrixXd times;
        if (!raw.read_raw_segment(data, times, fromSamp, toSamp)) {
            Log::info("  Warning: cannot read epoch at sample {}", eventSample);
            continue;
        }

        ++epochCount;

        // Write MAT file
        const std::string matFile = mne::utf8_from_path(dir / std::format("epoch_{:03}.mat", epochCount));
        MatrixXd sfreqMat(1, 1);
        sfreqMat(0, 0) = sfreq;
        if (auto written = write_mat_file(mne::path_from_utf8(matFile), "mne_epochs2mat", {{"data", data}, {"sfreq", sfreqMat}, {"times", timesMat}});
            !written) {
            Log::critical("{}", written.error().what());
            return 1;
        }

        Log::info("  Wrote epoch {} -> {} ({} x {})", epochCount, matFile, nChan, epochLen);
    }

    Log::info("Done. Extracted {} epochs for event ID {}.", epochCount, eventId);
    return 0;
}
