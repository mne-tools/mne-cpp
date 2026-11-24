//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    CLI event annotation tool for FIFF measurement files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
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
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
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

// Parses a number as QString::toDouble: an infinity when it overflows, 0 when it is no number
static double toDouble(std::string_view text)
{
    if (const std::optional<double> value = to_double(text)) {
        return *value;
    }
    // to_double rejects a number beyond the double range, for which QString returns an infinity
    const std::string number(trimmed(text));
    char* end = nullptr;
    const double parsed = std::strtod(number.c_str(), &end);
    return !number.empty() && end == number.c_str() + number.size() && std::isinf(parsed) ? parsed : 0.0;
}

//=============================================================================================================

struct EventEntry
{
    int sample;
    int before;
    int after;
    std::string comment;
};

//=============================================================================================================

static std::vector<EventEntry> readEventFile(const std::string& fileName)
{
    std::vector<EventEntry> events;
    std::ifstream file(path_from_utf8(fileName));
    if (!file)
        return events;

    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() < 3)
            continue;

        EventEntry ev;
        ev.sample = to_int(parts[0]).value_or(0);
        ev.before = to_int(parts[1]).value_or(0);
        ev.after = to_int(parts[2]).value_or(0);
        ev.comment = join(StringList(parts.begin() + 3, parts.end()), " ");
        events.push_back(ev);
    }
    return events;
}

//=============================================================================================================

static bool writeEventFile(const std::string& fileName, const std::vector<EventEntry>& events)
{
    std::ofstream out(path_from_utf8(fileName));
    if (!out) {
        Log::critical("Cannot open output file: {}", fileName);
        return false;
    }
    out << "# MNE event file (mne_dacq_annotator)\n";
    out << "# sample  before  after  [comment]\n";
    for (const EventEntry& ev : events) {
        out << ev.sample << "\t" << ev.before << "\t" << ev.after;
        if (!ev.comment.empty()) {
            out << "\t" << ev.comment;
        }
        out << "\n";
    }
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "CLI event annotation tool for FIFF measurement files.\n\n"
        "Create, list, and edit event annotations associated with MEG/EEG\n"
        "recordings. Events are stored in the standard MNE event file format\n"
        "(three-column: sample, before, after) with optional comment fields.\n\n"
        "The original MNE-C mne_dacq_annotator was a Motif/X11 GUI for\n"
        "real-time annotation during MEG acquisition. This CLI port provides\n"
        "the same annotation management as a scriptable command-line tool.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "Input FIFF measurement file.", "file", {}});
    parser.add_option({{"events"}, "Event file to read/edit (default: <meas>-eve.fif).", "file", {}});
    parser.add_option({{"add"}, "Add event: sample:before:after[:comment].", "spec", {}});
    parser.add_option({{"remove"}, "Remove event at sample number.", "sample", {}});
    parser.add_option({{"list"}, "List all events.", {}, {}});
    parser.add_option({{"out"}, "Output event file (default: overwrite --events).", "file", {}});
    parser.add_option({{"stim-ch"}, "Extract events from stimulus channel.", "name", {}});
    parser.add_option({{"thresh"}, "Threshold for stimulus channel extraction.", "value", "1.0"});
    parser.process(argc, argv, "mne_dacq_annotator", PROGRAM_VERSION);

    const std::string measFile = parser.value("meas");
    std::string eventsFile = parser.value("events");
    std::string outFile = parser.value("out");
    bool doList = parser.is_set("list");

    //=========================================================================
    // Read measurement info
    //=========================================================================
    float sfreq = 0.0f;

    if (!measFile.empty()) {
        FiffRawData rawData(path_from_utf8(measFile));
        if (rawData.info.empty()) {
            Log::critical("Cannot read measurement info from: {}", measFile);
            return 1;
        }
        sfreq = rawData.info.sfreq;
        Log::info("Measurement: {} channels, sfreq = {:.1f} Hz", rawData.info.nchan, sfreq);

        // Default events file
        if (eventsFile.empty()) {
            const std::filesystem::path measPath = path_from_utf8(measFile);
            const std::filesystem::path dir = measPath.parent_path();
            eventsFile = (dir.empty() ? std::string(".") : utf8_from_path(dir)) + "/" + utf8_from_path(measPath.stem()) + "-eve.fif";
        }
    }

    //=========================================================================
    // Extract events from stimulus channel
    //=========================================================================
    if (parser.is_set("stim-ch")) {
        if (measFile.empty()) {
            Log::critical("--meas is required with --stim-ch.");
            return 1;
        }

        const std::string stimName = parser.value("stim-ch");
        double thresh = toDouble(parser.value("thresh"));

        FiffRawData raw(path_from_utf8(measFile));

        // Find stimulus channel
        int stimIdx = -1;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].ch_name == stimName) {
                stimIdx = i;
                break;
            }
        }
        if (stimIdx < 0) {
            Log::critical("Stimulus channel '{}' not found.", stimName);
            return 1;
        }

        // Read all data for stimulus channel
        MatrixXd data;
        MatrixXd times;
        RowVectorXi sel(1);
        sel(0) = stimIdx;

        if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp, sel)) {
            Log::critical("Cannot read stimulus channel data.");
            return 1;
        }

        // Find threshold crossings (rising edges)
        std::vector<EventEntry> events;
        for (int i = 1; i < data.cols(); ++i) {
            double prev = data(0, i - 1);
            double curr = data(0, i);
            if (prev < thresh && curr >= thresh) {
                int eventId = static_cast<int>(std::round(curr));
                EventEntry ev;
                ev.sample = raw.first_samp + i;
                ev.before = 0;
                ev.after = eventId;
                events.push_back(ev);
            }
        }

        Log::info("Found {} events from stimulus channel '{}'", static_cast<long long>(events.size()), stimName);

        if (outFile.empty())
            outFile = eventsFile;
        if (outFile.empty()) {
            Log::critical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events))
            return 1;
        Log::info("Written events to: {}", outFile);
        return 0;
    }

    //=========================================================================
    // Load existing events
    //=========================================================================
    std::vector<EventEntry> events;
    std::error_code existsError;
    if (!eventsFile.empty() && std::filesystem::exists(path_from_utf8(eventsFile), existsError)) {
        events = readEventFile(eventsFile);
        Log::info("Loaded {} existing events from: {}", static_cast<long long>(events.size()), eventsFile);
    }

    //=========================================================================
    // Add events
    //=========================================================================
    if (parser.is_set("add")) {
        const std::string spec = parser.value("add");
        const StringList parts = split(spec, ':');
        if (parts.size() < 3) {
            Log::critical("Invalid event spec '{}'. Use sample:before:after[:comment].", spec);
            return 1;
        }

        EventEntry ev;
        ev.sample = to_int(parts[0]).value_or(0);
        ev.before = to_int(parts[1]).value_or(0);
        ev.after = to_int(parts[2]).value_or(0);
        ev.comment = join(StringList(parts.begin() + 3, parts.end()), ":");

        events.push_back(ev);
        Log::info("Added event: sample={}, before={}, after={}{}", ev.sample, ev.before, ev.after, ev.comment.empty() ? std::string() : ", comment=" + ev.comment);

        // Sort by sample
        std::sort(events.begin(), events.end(),
                  [](const EventEntry& a, const EventEntry& b) { return a.sample < b.sample; });
    }

    //=========================================================================
    // Remove events
    //=========================================================================
    if (parser.is_set("remove")) {
        int removeSample = to_int(parser.value("remove")).value_or(0);
        int removed = 0;
        for (int i = static_cast<int>(events.size()) - 1; i >= 0; --i) {
            if (events[i].sample == removeSample) {
                events.erase(events.begin() + i);
                removed++;
            }
        }
        Log::info("Removed {} event(s) at sample {}", removed, removeSample);
    }

    //=========================================================================
    // List events
    //=========================================================================
    if (doList) {
        Log::info("\n{:<10}  {:<8}  {:<8}  {}", "Sample", "Before", "After", "Comment");
        Log::info("{:<10}  {:<8}  {:<8}  {}", "------", "------", "-----", "-------");
        for (const EventEntry& ev : events) {
            std::string line = std::format("{:<10}  {:<8}  {:<8}", ev.sample, ev.before, ev.after);
            if (sfreq > 0) {
                line += std::format("  (t={:.3f} s)", ev.sample / static_cast<double>(sfreq));
            }
            if (!ev.comment.empty()) {
                line += "  " + ev.comment;
            }
            Log::info("{}", line);
        }
        Log::info("\nTotal: {} event(s)", static_cast<long long>(events.size()));
    }

    //=========================================================================
    // Write output
    //=========================================================================
    if (parser.is_set("add") || parser.is_set("remove")) {
        if (outFile.empty())
            outFile = eventsFile;
        if (outFile.empty()) {
            Log::critical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events))
            return 1;
        Log::info("Written {} events to: {}", static_cast<long long>(events.size()), outFile);
    }

    return 0;
}
