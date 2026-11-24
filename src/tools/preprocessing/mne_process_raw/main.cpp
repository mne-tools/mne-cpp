//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2004
 * @brief    mne_process_raw main entry point.
 *
 *           Batch processing of raw MEG/EEG data.
 *           Ported from mne_browse_raw/mne_process_raw (MNE-C) by Matti Hamalainen.
 *
 *           Features:
 *           - Filter and decimate raw data
 *           - Detect and save events from trigger channels
 *           - Compute averages from description files
 *           - Compute noise covariance matrices from description files
 *           - Create SSP (Signal-Space Projection) operators
 *           - Grand averaging across multiple files
 *
 *           Compatible with MNE-C command-line options.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "batchprocessor.h"
#include <mne/mne/mne_process_description.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEPROCESSRAWAPP;
using namespace mne;

//=============================================================================================================
// VERSION
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// HELPERS
//=============================================================================================================

// Parses a number as QString::toDouble: an infinity when it overflows, 0 with @p ok false when it is no number
static double toDouble(std::string_view text, bool& ok)
{
    if (const std::optional<double> value = to_double(text)) {
        ok = true;
        return *value;
    }
    ok = false;
    // to_double rejects a number beyond the double range, for which QString returns an infinity
    const std::string number(trimmed(text));
    char* end = nullptr;
    const double parsed = std::strtod(number.c_str(), &end);
    return !number.empty() && end == number.c_str() + number.size() && std::isinf(parsed) ? parsed : 0.0;
}

//=============================================================================================================

// Parses a number as QString::toFloat: an infinity when it overflows the float range, a signed 0 when it underflows
static float toFloat(std::string_view text, bool& ok)
{
    const double value = toDouble(text, ok);
    if (std::isfinite(value) && std::abs(value) > std::numeric_limits<float>::max()) {
        ok = false;
        return value > 0 ? std::numeric_limits<float>::infinity() : -std::numeric_limits<float>::infinity();
    }
    const auto single = static_cast<float>(value);
    if (value != 0.0 && single == 0.0f) {
        ok = false;
    }
    return single;
}

//=============================================================================================================

static float toFloat(std::string_view text)
{
    bool ok = false;
    return toFloat(text, ok);
}

//=============================================================================================================

// Parses an unsigned decimal or 0x-prefixed hexadecimal mask; 0 when the text is no such number
static unsigned int toMask(std::string_view text)
{
    int base = 10;
    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text.remove_prefix(2);
        base = 16;
    }
    text = trimmed(text);
    if (text.starts_with('+'))
        text.remove_prefix(1);
    unsigned int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (text.empty() || error != std::errc() || end != text.data() + text.size())
        return 0;
    return value;
}

//=============================================================================================================

/**
 * Prints a richly formatted, grouped help text to stdout.
 */
static void printHelp()
{
    std::print("{}",
               "\n"
               "Usage: mne_process_raw [options]\n"
               "\n"
               "  Process raw MEG/EEG data — filtering, averaging, covariance\n"
               "  computation, event detection, SSP operator creation, and decimation.\n"
               "\n"
               "  Ported from MNE-C mne_process_raw by Matti Hamalainen.\n"
               "\n"
               "General:\n"
               "  --help                    Print this help and exit.\n"
               "  --version                 Print version information and exit.\n"
               "  --cd <dir>                Change working directory before processing.\n"
               "\n"
               "Input:\n"
               "  --raw <file>              Raw data file to process.  May be given\n"
               "                            multiple times for grand averaging.\n"
               "  --grad <number>           Desired software gradient compensation order.\n"
               "  --allowmaxshield          Allow loading of unprocessed MaxShield data.\n"
               "\n"
               "Events:\n"
               "  --events <file>           Load event list from file.\n"
               "  --eventsout <file>        Save detected events to file.\n"
               "  --allevents               Detect all trigger transitions, not just\n"
               "                            rising edges.\n"
               "  --digtrig <name>          Trigger channel name (default: 'STI 014').\n"
               "                            Use underscores for spaces (e.g. STI_014).\n"
               "  --digtrigmask <value>     Trigger channel bit mask (decimal or 0xHex).\n"
               "\n"
               "Projection (SSP):\n"
               "  --proj <file>             Load SSP vectors from file (repeatable).\n"
               "  --projon                  Activate SSP when processing.\n"
               "  --projoff                 Deactivate SSP when processing.\n"
               "  --makeproj                Compute new SSP operators from the data.\n"
               "  --projevent <code>        Event code for projector computation.\n"
               "  --projtmin <sec>          Start time for projector window.\n"
               "  --projtmax <sec>          End time for projector window.\n"
               "  --projngrad <n>           Gradiometer SSP components  (default: 2).\n"
               "  --projnmag <n>            Magnetometer SSP components (default: 2).\n"
               "  --projneeg <n>            EEG SSP components          (default: 0).\n"
               "  --projgradrej <fT/cm>     Grad rejection for SSP      (default: 2000).\n"
               "  --projmagrej <fT>         Mag  rejection for SSP      (default: 3000).\n"
               "  --projeegrej <uV>         EEG  rejection for SSP      (default: 0).\n"
               "  --saveprojtag <tag>       Tag appended to SSP output filenames.\n"
               "\n"
               "Filtering:\n"
               "  --filtersize <n>          FFT length for filtering  (default: 8192).\n"
               "  --highpass <Hz>           High-pass corner frequency.\n"
               "  --lowpass <Hz>            Low-pass  corner frequency.\n"
               "  --highpassw <Hz>          High-pass transition bandwidth.\n"
               "  --lowpassw <Hz>           Low-pass  transition bandwidth.\n"
               "  --filteroff               Do not filter the data.\n"
               "\n"
               "Output / Decimation:\n"
               "  --save <file>             Save processed raw data to file.\n"
               "  --decim <factor>          Decimation factor (default: 1).\n"
               "  --split <MB>              Split output at this file size (MB).\n"
               "  --anon                    Omit subject information from output.\n"
               "  --savehere                Write output to current dir instead of\n"
               "                            the raw file's directory.\n"
               "\n"
               "Averaging:\n"
               "  --ave <file>              Average description file (repeatable).\n"
               "  --saveavetag <tag>        Tag appended to average output filenames.\n"
               "  --gave <file>             Grand average output file.\n"
               "\n"
               "Covariance:\n"
               "  --cov <file>              Covariance description file (repeatable).\n"
               "  --savecovtag <tag>        Tag appended to covariance output filenames.\n"
               "  --gcov <file>             Grand covariance output file.\n"
               "\n"
               "Examples:\n"
               "\n"
               "  Detect events and save them:\n"
               "    mne_process_raw --raw data.fif --eventsout events.fif\n"
               "\n"
               "  Band-pass filter and save:\n"
               "    mne_process_raw --raw data.fif --highpass 1 --lowpass 40 \\\n"
               "                    --save filtered.fif\n"
               "\n"
               "  Compute averages from a description file:\n"
               "    mne_process_raw --raw data.fif --ave auditory.ave\n"
               "\n"
               "  Compute noise covariance:\n"
               "    mne_process_raw --raw data.fif --cov noise.cov\n"
               "\n"
               "  Grand average across subjects:\n"
               "    mne_process_raw --raw subj1.fif --raw subj2.fif \\\n"
               "                    --ave paradigm.ave --gave grand_avg.fif\n"
               "\n"
               "  Decimate and save with projection:\n"
               "    mne_process_raw --raw data.fif --proj ssp.fif --projon \\\n"
               "                    --decim 4 --save decimated.fif\n"
               "\n"
               "  Create SSP operators from empty-room data:\n"
               "    mne_process_raw --raw empty_room.fif --makeproj \\\n"
               "                    --projngrad 3 --projnmag 3\n"
               "\n");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The main entry point of mne_process_raw.
 *
 * @param[in] argc  Number of command-line arguments.
 * @param[in] argv  Array of command-line argument strings.
 * @return Application exit code.
 */
int main(int argc, char* argv[])
{
    //=========================================================================
    // Set up command-line parser
    //=========================================================================
    CommandLineParser parser(
        "Process raw MEG/EEG data.\n\n"
        "Ported from MNE-C mne_process_raw by Matti Hamalainen.\n"
        "Supports filtering, averaging, covariance computation, event detection,\n"
        "SSP operator creation, and decimation of raw data files.");
    parser.add_version_option();

    // Custom help option (replaces the default one with grouped output)
    parser.add_option({{"h", "help"}, "Print this help and exit.", {}, {}});

    // --- Working directory ---
    parser.add_option({{"cd"}, "Change working directory.", "dir", {}});

    // --- Raw data files ---
    parser.add_option({{"raw"}, "Input raw data file (may be specified multiple times).", "file", {}});

    // --- Gradient compensation ---
    parser.add_option({{"grad"}, "Desired software gradient compensation order.", "number", {}});

    // --- MaxShield ---
    parser.add_option({{"allowmaxshield"}, "Allow unprocessed MaxShield data.", {}, {}});

    // --- Event I/O ---
    parser.add_option({{"events"}, "Load events from this file.", "file", {}});
    parser.add_option({{"eventsout"}, "Save detected events to this file.", "file", {}});
    parser.add_option({{"allevents"}, "Save all trigger line transitions (not just rising edges).", {}, {}});

    // --- Projection ---
    parser.add_option({{"proj"}, "Load SSP projection from this file (may be specified multiple times).", "file", {}});
    parser.add_option({{"projon"}, "Apply SSP to the data.", {}, {}});
    parser.add_option({{"projoff"}, "Do not apply SSP to the data.", {}, {}});
    parser.add_option({{"makeproj"}, "Create new SSP operators from the data.", {}, {}});
    parser.add_option({{"projevent"}, "Event code for projector creation.", "number", {}});
    parser.add_option({{"projtmin"}, "Start time for projector calculation (sec).", "time", {}});
    parser.add_option({{"projtmax"}, "End time for projector calculation (sec).", "time", {}});
    parser.add_option({{"projngrad"}, "Number of gradiometer components for SSP (default=2).", "value", "2"});
    parser.add_option({{"projnmag"}, "Number of magnetometer components for SSP (default=2).", "value", "2"});
    parser.add_option({{"projneeg"}, "Number of EEG components for SSP (default=0).", "value", "0"});
    parser.add_option({{"projgradrej"}, "Grad rejection for SSP (fT/cm, default=2000).", "value", "2000"});
    parser.add_option({{"projmagrej"}, "Mag rejection for SSP (fT, default=3000).", "value", "3000"});
    parser.add_option({{"projeegrej"}, "EEG rejection for SSP (uV, default=0).", "value", "0"});
    parser.add_option({{"saveprojtag"}, "Tag for projection output files.", "tag", {}});

    // --- Filter ---
    parser.add_option({{"filtersize"}, "Filter length (default=8192).", "size", "8192"});
    parser.add_option({{"highpass"}, "Highpass corner frequency (Hz).", "freq", {}});
    parser.add_option({{"lowpass"}, "Lowpass corner frequency (Hz).", "freq", {}});
    parser.add_option({{"highpassw"}, "Highpass transition width (Hz).", "freq", {}});
    parser.add_option({{"lowpassw"}, "Lowpass transition width (Hz).", "freq", {}});
    parser.add_option({{"filteroff"}, "Do not filter the data.", {}, {}});

    // --- Save/Decimate ---
    parser.add_option({{"save"}, "Save filtered/decimated raw data to this file.", "file", {}});
    parser.add_option({{"anon"}, "Omit subject info from output.", {}, {}});
    parser.add_option({{"decim"}, "Decimation factor for saving (default=1).", "factor", "1"});
    parser.add_option({{"split"}, "Split output files at this size (MB).", "size", {}});

    // --- Averaging ---
    parser.add_option({{"ave"}, "Average description file (may be specified multiple times).", "file", {}});
    parser.add_option({{"saveavetag"}, "Tag for average output files.", "tag", {}});
    parser.add_option({{"gave"}, "Grand average output file.", "file", {}});

    // --- Covariance ---
    parser.add_option({{"cov"}, "Covariance description file (may be specified multiple times).", "file", {}});
    parser.add_option({{"savecovtag"}, "Tag for covariance output files.", "tag", {}});
    parser.add_option({{"gcov"}, "Grand average covariance output file.", "file", {}});

    // --- Save location ---
    parser.add_option({{"savehere"}, "Save output files in current directory instead of raw data directory.", {}, {}});

    // --- Trigger ---
    parser.add_option({{"digtrig"}, "Digital trigger channel name (default='STI 014').", "name", "STI 014"});
    parser.add_option({{"digtrigmask"}, "Mask for digital trigger channel (decimal or 0x hex).", "value", {}});

    //=========================================================================
    // Parse command-line arguments
    //=========================================================================
    bool parsed = parser.parse(argc, argv);
    if (!parsed && parser.error_text().starts_with("Missing value after")) {
        // Only the last argument can miss its value; it is ignored, as before
        parsed = parser.parse(argc - 1, argv);
    }
    if (!parsed) {
        // The parser stops at the first problem; an unknown option is reported by name as before
        const std::string& error = parser.error_text();
        const std::string_view prefix = "Unknown option '";
        if (error.starts_with(prefix) && error.ends_with("'."))
            Log::critical("Unknown option: {}", std::string_view(error).substr(prefix.size(), error.size() - prefix.size() - 2));
        else
            Log::critical("{}", error);
        printHelp();
        return 1;
    }

    if (parser.is_set("help")) {
        printHelp();
        return 0;
    }
    if (parser.is_set("version")) {
        std::println("mne_process_raw {}", PROGRAM_VERSION);
        return 0;
    }

    //=========================================================================
    // Change working directory if requested
    //=========================================================================
    if (parser.is_set("cd")) {
        std::error_code error;
        std::filesystem::current_path(path_from_utf8(parser.value("cd")), error);
        Log::info("Working directory changed to: {}", utf8_from_path(std::filesystem::current_path(error)));
    }

    //=========================================================================
    // Populate ProcessingSettings
    //=========================================================================
    ProcessingSettings settings;

    // Raw files
    settings.rawFiles = parser.values("raw");

    // Event files
    settings.eventFiles = parser.values("events");
    settings.eventsOutFiles = parser.values("eventsout");

    // Trigger settings
    // Underscores stand for spaces, as in MNE-C
    settings.digTrigger = parser.value("digtrig");
    std::ranges::replace(settings.digTrigger, '_', ' ');

    if (parser.is_set("digtrigmask"))
        settings.digTriggerMask = toMask(parser.value("digtrigmask"));

    // Projection settings
    settings.projFiles = parser.values("proj");
    if (parser.is_set("projon"))
        settings.projOn = 1;
    else if (parser.is_set("projoff"))
        settings.projOn = 0;

    settings.makeProj = parser.is_set("makeproj");
    settings.projEvent = to_int(parser.value("projevent")).value_or(0);
    settings.projTmin = toFloat(parser.value("projtmin"));
    settings.projTmax = toFloat(parser.value("projtmax"));
    settings.projNGrad = to_int(parser.value("projngrad")).value_or(0);
    settings.projNMag = to_int(parser.value("projnmag")).value_or(0);
    settings.projNEeg = to_int(parser.value("projneeg")).value_or(0);
    settings.projGradReject = toFloat(parser.value("projgradrej")) * 1e-13f;
    settings.projMagReject = toFloat(parser.value("projmagrej")) * 1e-15f;
    settings.projEegReject = toFloat(parser.value("projeegrej")) * 1e-6f;
    settings.saveProjTag = parser.value("saveprojtag");
    if (!settings.saveProjTag.empty())
        settings.makeProj = true;

    // Filter settings
    settings.filter.filterSize = to_int(parser.value("filtersize")).value_or(0);
    if (parser.is_set("highpass"))
        settings.filter.highpass = toFloat(parser.value("highpass"));
    if (parser.is_set("lowpass"))
        settings.filter.lowpass = toFloat(parser.value("lowpass"));
    if (parser.is_set("highpassw"))
        settings.filter.highpassWidth = toFloat(parser.value("highpassw"));
    if (parser.is_set("lowpassw"))
        settings.filter.lowpassWidth = toFloat(parser.value("lowpassw"));
    settings.filter.filterOn = !parser.is_set("filteroff");

    // Save settings
    settings.saveFiles = parser.values("save");
    settings.decimation = to_int(parser.value("decim")).value_or(0);
    if (parser.is_set("split"))
        settings.splitSize = static_cast<long>(toFloat(parser.value("split")) * 1024 * 1024);

    // Averaging settings
    settings.aveFiles = parser.values("ave");
    settings.saveAveTag = parser.value("saveavetag");
    settings.grandAveFile = parser.value("gave");

    // Covariance settings
    settings.covFiles = parser.values("cov");
    settings.saveCovTag = parser.value("savecovtag");
    settings.grandCovFile = parser.value("gcov");

    // Other settings
    settings.saveHere = parser.is_set("savehere");

    //=========================================================================
    // Validate required options
    //=========================================================================
    if (settings.rawFiles.empty()) {
        Log::critical("Error: At least one --raw file must be specified.");
        printHelp();
        return 1;
    }

    //=========================================================================
    // Run batch processing
    //=========================================================================
    return BatchProcessor::run(settings);
}
