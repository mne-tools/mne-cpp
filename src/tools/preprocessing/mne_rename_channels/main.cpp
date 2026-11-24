//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Rename channels in FIFF files using an alias file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>
#include <mne/fiff/fiff_ch_info.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstddef>
#include <format>
#include <fstream>
#include <optional>
#include <print>
#include <string>
#include <string_view>
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

struct ChannelAlias
{
    std::string from;
    std::string to;
    int toKind; // -1 means do not change kind
};

//=============================================================================================================

static std::string explainKind(int kind)
{
    switch (kind) {
        case FIFFV_MEG_CH:
            return "MEG";
        case FIFFV_MCG_CH:
            return "MCG";
        case FIFFV_EEG_CH:
            return "EEG";
        case FIFFV_EOG_CH:
            return "EOG";
        case FIFFV_EMG_CH:
            return "EMG";
        case FIFFV_ECG_CH:
            return "ECG";
        case FIFFV_MISC_CH:
            return "MISC";
        case FIFFV_STIM_CH:
            return "STI";
        case FIFFV_RESP_CH:
            return "RESP";
        default:
            return "Unknown";
    }
}

//=============================================================================================================

// Byte length of the first @p maxUnits UTF-16 code units of the UTF-8 @p text, never splitting a character
static std::size_t utf16Prefix(std::string_view text, std::size_t maxUnits)
{
    std::size_t bytes = 0;
    std::size_t units = 0;
    while (bytes < text.size()) {
        const auto lead = static_cast<unsigned char>(text[bytes]);
        const std::size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2
            : lead < 0xF0                                        ? 3
                                                                 : 4;
        const std::size_t width = length == 4 ? 2 : 1;
        if (units + width > maxUnits)
            break;
        units += width;
        bytes += length;
    }
    return std::min(bytes, text.size());
}

//=============================================================================================================

static std::vector<ChannelAlias> readAliases(const std::string& filename, bool reverse)
{
    std::vector<ChannelAlias> aliases;
    std::ifstream file(path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open alias file: {}", filename);
        return aliases;
    }
    std::string raw;
    while (std::getline(file, raw)) {
        const std::string_view line = trimmed(raw);
        if (line.empty() || line.starts_with('#'))
            continue;

        const StringList parts = split(line, ':');
        if (parts.size() < 2) {
            Log::critical("Alias line should contain a colon: {}", line);
            return {};
        }

        ChannelAlias a;
        if (reverse) {
            a.from = trimmed(parts[1]);
            a.to = trimmed(parts[0]);
        } else {
            a.from = trimmed(parts[0]);
            a.to = trimmed(parts[1]);
        }

        a.toKind = -1;
        if (parts.size() >= 3) {
            const std::optional<int> kind = to_int(trimmed(parts[2]));
            if (!kind) {
                Log::critical("Cannot interpret new kind: {}", parts[2]);
                return {};
            }
            a.toKind = *kind;
        }

        if (a.to.empty()) {
            Log::critical("Destination name missing (from = {})", a.from);
            return {};
        }

        // Truncate to 15 chars (FIFF channel name limit)
        if (const std::size_t prefix = utf16Prefix(a.to, 15); prefix < a.to.size()) {
            a.to.resize(prefix);
            Log::warning("Warning: name truncated to <{}>", a.to);
        }

        aliases.push_back(a);
    }
    return aliases;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Give new names to selected channels in a FIFF file.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"fif"}, "FIFF file to modify.", "name", {}});
    parser.add_option({{"alias"}, "Alias file (<old>:<new>[:<kind>]).", "name", {}});
    parser.add_option({{"revert", "reverse"}, "Swap old/new in the alias file.", {}, {}});
    parser.process(argc, argv, "mne_rename_channels", PROGRAM_VERSION);

    const std::string fifName = parser.value("fif");
    const std::string aliasName = parser.value("alias");
    bool reverse = parser.is_set("revert");

    if (fifName.empty() || aliasName.empty()) {
        Log::critical("Both --fif and --alias are required.");
        std::print("{}", parser.help_text("mne_rename_channels"));
        return 1;
    }

    Log::info("fif file   : {}", fifName);
    Log::info("alias file : {}", aliasName);
    if (reverse)
        Log::info("Exchange the roles of old and new channel names in the alias file");

    const std::vector<ChannelAlias> aliases = readAliases(aliasName, reverse);
    if (aliases.empty()) {
        Log::critical("No aliases read.");
        return 1;
    }
    Log::info("{} aliases read from {}", static_cast<long long>(aliases.size()), aliasName);

    // Open FIFF file for in-place update
    auto stream = FiffStream::open_update(path_from_utf8(fifName));
    if (!stream) {
        Log::critical("Cannot open file for update: {}", fifName);
        return 1;
    }

    int nMod = 0;
    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k].kind == FIFF_CH_INFO) {
            std::optional<FiffTag> tag;
            tag = stream->read_tag(stream->dir()[k].pos);
            FiffChInfo ch = tag->to_ch_info();

            for (const ChannelAlias& a : aliases) {
                if (ch.ch_name == a.from) {
                    std::string line = std::format("{} -> {}", ch.ch_name, a.to);
                    nMod++;
                    ch.ch_name = a.to;
                    if (a.toKind > 0) {
                        line += std::format(" ({} -> {})", explainKind(ch.kind), explainKind(a.toKind));
                        ch.kind = a.toKind;
                    }
                    Log::info("{}", line);
                    stream->byte_stream().seek(stream->dir()[k].pos);
                    stream->write_ch_info(ch);
                    break;
                }
            }
        }
    }

    stream->close();
    Log::info("{} changes done", nMod);
    Log::info("{} processed.", fifName);

    return 0;
}
