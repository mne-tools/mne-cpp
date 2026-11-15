//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutloader.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Implementation of the @c .elc reader declared in @ref layoutloader.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutloader.h"

#include <base/log.h>

#include <algorithm>
#include <fstream>
#include <sstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

enum class Block
{
    Header,
    Positions3d,
    Positions2d,
    Labels
};

// The trailing numbers of a position line, after an optional "name :" prefix.
std::optional<std::vector<double>> coordinates(std::string_view line, std::string* name)
{
    if (const auto colon = line.find(':'); colon != std::string_view::npos) {
        if (name)
            *name = std::string(trimmed(line.substr(0, colon)));
        line.remove_prefix(colon + 1);
    }
    std::vector<double> values;
    for (const std::string& field : split(line, ' ', true)) {
        const auto value = toDouble(field);
        if (!value)
            return std::nullopt;
        values.push_back(*value);
    }
    return values;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<ElectrodeLayout> LayoutLoader::read_elc(const std::filesystem::path& path)
{
    if (toLower(path.extension().string()) != ".elc")
        return std::nullopt;

    std::ifstream file(path);
    if (!file) {
        Log::debug("Cannot open the electrode file {}", path.string());
        return std::nullopt;
    }
    std::stringstream text;
    text << file.rdbuf();
    return parse_elc(text.str());
}

//=============================================================================================================

std::optional<ElectrodeLayout> LayoutLoader::parse_elc(std::string_view text)
{
    ElectrodeLayout layout;
    StringList prefixNames;
    StringList labels;
    Block block = Block::Header;

    for (std::string line : split(text, '\n')) {
        std::replace(line.begin(), line.end(), '\t', ' ');
        const std::string_view content = trimmed(line);
        if (content.empty() || content.front() == '#')
            continue;

        const StringList fields = split(content, ' ', true);
        const std::string_view keyword = fields.front();
        if (keyword == "Positions") {
            block = Block::Positions3d;
        } else if (keyword == "Positions2D") {
            block = Block::Positions2d;
        } else if (keyword == "Labels") {
            block = Block::Labels;
        } else if (keyword == "UnitPosition") {
            if (fields.size() > 1)
                layout.unit = fields[1];
        } else if (keyword.starts_with("NumberPositions") || keyword == "ReferenceLabel") {
            continue;
        } else if (block == Block::Labels) {
            labels.insert(labels.end(), fields.begin(), fields.end());
        } else if (block == Block::Positions3d) {
            std::string name;
            const auto values = coordinates(content, &name);
            if (!values || values->size() != 3)
                return std::nullopt;
            layout.positions_3d.emplace_back((*values)[0], (*values)[1], (*values)[2]);
            prefixNames.push_back(std::move(name));
        } else if (block == Block::Positions2d) {
            const auto values = coordinates(content, nullptr);
            if (!values || values->size() != 2)
                return std::nullopt;
            layout.positions_2d.emplace_back((*values)[0], (*values)[1]);
        }
    }

    layout.names = labels.empty() ? std::move(prefixNames) : std::move(labels);
    if (layout.positions_3d.empty() || layout.names.size() != layout.positions_3d.size())
        return std::nullopt;
    return layout;
}
