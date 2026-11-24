//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert CTF marker files to MNE event format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <fstream>
#include <ostream>
#include <print>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct CtfMarker
{
    std::string name;
    int eventId;
    std::vector<int> samples;

    bool operator==(const CtfMarker& other) const
    {
        return name == other.name && eventId == other.eventId;
    }
};

//=============================================================================================================

/**
 * Parse CTF MarkerFile format.
 *
 * The format consists of marker definitions with:
 *   MARKER CLASSIFICATION
 *   NAME: marker_name
 *   COMMENT:
 *   COLOR: color_name
 *   EDITABLE: Yes/No
 *   COPY DATA TO: No
 *   CLASSID: N
 *   NUMBER OF MARKERS: M
 *   LIST OF MARKERS:
 *   TRIAL NUMBER       TIME FROM SYNC POINT (in seconds)
 *   trial_num           time
 *   ...
 */
static std::vector<CtfMarker> parseCtfMarkers(const std::string& filename)
{
    std::vector<CtfMarker> markers;
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        mne::Log::critical("Cannot open CTF marker file: {}", filename);
        return markers;
    }

    CtfMarker current{};
    int classId = 0;
    bool inList = false;
    int nMarkers = 0;

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string line(mne::trimmed(rawLine));

        if (line.starts_with("NAME:")) {
            current = CtfMarker{};
            current.name = std::string(mne::trimmed(std::string_view(line).substr(5)));
            inList = false;
            continue;
        }

        if (line.starts_with("CLASSID:")) {
            classId = mne::to_int(std::string_view(line).substr(8)).value_or(0);
            current.eventId = classId;
            continue;
        }

        if (line.starts_with("NUMBER OF MARKERS:")) {
            nMarkers = mne::to_int(std::string_view(line).substr(18)).value_or(0);
            continue;
        }

        if (line.starts_with("LIST OF MARKERS:")) {
            inList = true;
            // Skip header line
            std::string header;
            std::getline(in, header);
            continue;
        }

        if (inList && !line.empty() && !line.starts_with("MARKER")) {
            // Parse trial_number and time
            const std::vector<std::string_view> parts = mne::split_whitespace(line);
            if (parts.size() >= 2) {
                // The sample is derived from the trial number and time;
                // for simplicity, we use the time as a sample index if it's an integer,
                // or store trial_number as sample position
                int sample = mne::to_int(parts[0]).value_or(0);
                current.samples.push_back(sample);
            }

            if (std::ssize(current.samples) >= nMarkers && nMarkers > 0) {
                inList = false;
                if (!current.name.empty()) {
                    markers.push_back(current);
                    current = CtfMarker{};
                }
            }
            continue;
        }

        // Start of new marker classification block
        if (line.starts_with("MARKER CLASSIFICATION")) {
            if (!current.name.empty() && !current.samples.empty()) {
                markers.push_back(current);
            }
            current = CtfMarker{};
            inList = false;
        }
    }

    // Add last marker if pending
    if (!current.name.empty() && !current.samples.empty() &&
        std::ranges::find(markers, current) == markers.end()) {
        markers.push_back(current);
    }

    return markers;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    mne::CommandLineParser parser("Convert CTF marker files to MNE event format.\n\n"
                                  "Output format: sample 0 event_id (one event per line).");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"markers"}, "CTF marker file.", "file", {}});
    parser.add_option({{"out"}, "Output event file (text format).", "file", {}});
    parser.process(argc, argv, "mne_convert_ctf_markers", PROGRAM_VERSION);

    const std::string markersFile = parser.value("markers");
    const std::string outFile = parser.value("out");

    if (markersFile.empty()) {
        mne::Log::critical("--markers is required.");
        return 1;
    }
    if (outFile.empty()) {
        mne::Log::critical("--out is required.");
        return 1;
    }

    // Parse CTF markers
    std::vector<CtfMarker> markers = parseCtfMarkers(markersFile);
    if (markers.empty()) {
        mne::Log::critical("No markers parsed from: {}", markersFile);
        return 1;
    }

    // Write MNE event format
    std::ofstream out(mne::path_from_utf8(outFile));
    if (!out) {
        mne::Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }

    int totalEvents = 0;

    for (const CtfMarker& marker : markers) {
        mne::Log::info("Marker '{}' (id={}): {} events", marker.name, marker.eventId, static_cast<long long>(marker.samples.size()));

        for (int sample : marker.samples) {
            std::print(out, "{} 0 {}\n", sample, marker.eventId);
            ++totalEvents;
        }
    }

    out.close();
    mne::Log::info("Written {} events to: {}", totalEvents, outFile);
    return 0;
}
