//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     selectionio.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2014
 * @brief    Implementation of the MNE @c .sel and Brainstorm @c .mon parsers declared in @ref selectionio.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionio.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <fstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

bool has_extension(const std::filesystem::path& path, std::string_view extension)
{
    return to_lower(path.extension().string()) == extension;
}

// A line without its Windows line end.
std::string_view without_cr(std::string_view line)
{
    return !line.empty() && line.back() == '\r' ? line.substr(0, line.size() - 1) : line;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<std::vector<ChannelSelection>> SelectionIO::read_sel(const std::filesystem::path& path)
{
    if (!has_extension(path, ".sel"))
        return std::nullopt;
    std::ifstream file(path);
    if (!file) {
        Log::debug("Cannot open the selection file {}", path.string());
        return std::nullopt;
    }

    std::vector<ChannelSelection> groups;
    for (std::string line; std::getline(file, line);) {
        const std::string_view content = without_cr(line);
        const auto colon = content.find(':');
        if (content.find('%') != std::string_view::npos || colon == std::string_view::npos)
            continue;
        groups.push_back({std::string(content.substr(0, colon)), split(content.substr(colon + 1), '|', true)});
    }
    return groups;
}

//=============================================================================================================

std::optional<ChannelSelection> SelectionIO::read_mon(const std::filesystem::path& path)
{
    if (!has_extension(path, ".mon"))
        return std::nullopt;
    std::ifstream file(path);
    if (!file) {
        Log::debug("Cannot open the montage file {}", path.string());
        return std::nullopt;
    }

    ChannelSelection group;
    std::string line;
    std::getline(file, line);
    group.name = without_cr(line);
    while (std::getline(file, line)) {
        if (const auto colon = line.find(':'); colon != std::string::npos)
            group.channels.emplace_back(trimmed(std::string_view(line).substr(0, colon)));
    }
    return group;
}

//=============================================================================================================

bool SelectionIO::write_sel(const std::filesystem::path& path, std::span<const ChannelSelection> groups)
{
    if (!has_extension(path, ".sel"))
        return false;
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        Log::debug("Cannot write the selection file {}", path.string());
        return false;
    }
    for (const ChannelSelection& group : groups) {
        file << group.name << ':';
        for (const std::string& channel : group.channels)
            file << channel << '|';
        file << "\n\n";
    }
    return static_cast<bool>(file);
}

//=============================================================================================================

bool SelectionIO::write_mon(const std::filesystem::path& directory, std::span<const ChannelSelection> groups)
{
    for (const ChannelSelection& group : groups) {
        // The group name becomes a file name, so it must not reach outside the directory
        if (group.name.empty() || group.name.find_first_of("/\\:") != std::string::npos) {
            Log::debug("Cannot use the group name '{}' as a file name", group.name);
            return false;
        }
        const std::filesystem::path fileName = path_from_utf8(group.name + ".mon");
        std::ofstream file(directory / fileName, std::ios::binary);
        if (!file) {
            Log::debug("Cannot write the montage file {}", (directory / fileName).string());
            return false;
        }
        file << group.name << '\n';
        for (const std::string& channel : group.channels)
            file << channel << " : " << channel << '\n';
        if (!file)
            return false;
    }
    return true;
}
