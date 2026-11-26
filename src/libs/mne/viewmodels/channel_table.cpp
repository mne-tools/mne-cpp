//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_table.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of ChannelTable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_table.h"
#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cctype>
#include <format>
#include <string_view>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viewmodels;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

constexpr std::array<std::string_view, ChannelTable::column_count> kHeaders = {
    "Data number", "Original name", "Alias", "Mapped layout name", "Channel kind", "MEG type", "Unit",
    "Position", "Digitizer (cm)", "Active filter", "Coil Type", "Bad channel", "# of Compensators"};

/** Removes every @p kind and the characters - _ / | and whitespace from @p name in one left-to-right pass. */
std::string layout_number(std::string_view name, std::string_view kind)
{
    std::string residual;
    for (std::size_t i = 0; i < name.size();) {
        if (name.substr(i, kind.size()) == kind) {
            i += kind.size();
        } else if (std::string_view("-_/|").find(name[i]) != std::string_view::npos || std::isspace(static_cast<unsigned char>(name[i]))) {
            ++i;
        } else {
            residual += name[i++];
        }
    }
    return residual;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelTable::ChannelTable()
: m_info(std::make_shared<FiffInfo>())
{
}

//=============================================================================================================

std::string ChannelTable::header(Column column)
{
    return std::string(kHeaders.at(static_cast<std::size_t>(column)));
}

//=============================================================================================================

void ChannelTable::set_info(std::shared_ptr<FiffInfo> info)
{
    m_info = info ? std::move(info) : std::make_shared<FiffInfo>();
    map_layout();
}

//=============================================================================================================

void ChannelTable::set_layout(std::map<std::string, std::array<double, 2>> layout)
{
    m_layout = std::move(layout);
    map_layout();
}

//=============================================================================================================

void ChannelTable::clear()
{
    m_info = std::make_shared<FiffInfo>();
    m_layout.clear();
    m_aliases.clear();
    m_mappedNames.clear();
}

//=============================================================================================================

int ChannelTable::row_count() const
{
    return static_cast<int>(m_info->chs.size());
}

//=============================================================================================================

std::string ChannelTable::text(int row, Column column) const
{
    if (row < 0 || row >= row_count()) {
        return {};
    }

    const FiffChInfo& ch = m_info->chs[static_cast<std::size_t>(row)];
    const auto at = [row](const std::vector<std::string>& names) {
        return row < std::ssize(names) ? names[static_cast<std::size_t>(row)] : std::string();
    };

    switch (column) {
        case Column::Number:
            return std::to_string(row);
        case Column::OriginalName:
            return ch.ch_name;
        case Column::Alias:
            return at(m_aliases);
        case Column::MappedLayoutName:
            return at(m_mappedNames);
        case Column::Kind:
            return std::to_string(ch.kind);
        case Column::MegType:
            return meg_type(row);
        case Column::Unit:
            return std::to_string(ch.unit);
        case Column::Position: {
            const std::array<double, 2> point = position(row);
            return std::format("({:g}|{:g})", point[0], point[1]);
        }
        case Column::Digitizer: {
            const Eigen::Vector3f point = digitizer_cm(row);
            return std::format("({:g}|{:g}|{:g})", point.x(), point.y(), point.z());
        }
        case Column::ActiveFilter:
            return {};
        case Column::CoilType:
            return std::to_string(ch.chpos.coil_type);
        case Column::Bad:
            return is_bad(row) ? "true" : "false";
        case Column::Compensators:
            return std::to_string(m_info->comps.size());
    }
    return {};
}

//=============================================================================================================

std::string ChannelTable::meg_type(int row) const
{
    const FiffChInfo& ch = m_info->chs.at(static_cast<std::size_t>(row));
    if (ch.kind == FIFFV_MEG_CH) {
        if (ch.unit == FIFF_UNIT_T_M) {
            return "MEG_grad";
        }
        if (ch.unit == FIFF_UNIT_T) {
            return "MEG_mag";
        }
    }
    return "non_MEG";
}

//=============================================================================================================

std::array<double, 2> ChannelTable::position(int row) const
{
    if (row < 0 || row >= std::ssize(m_mappedNames)) {
        return {0.0, 0.0};
    }
    const auto it = m_layout.find(m_mappedNames[static_cast<std::size_t>(row)]);
    return it == m_layout.end() ? std::array<double, 2>{0.0, 0.0} : it->second;
}

//=============================================================================================================

Eigen::Vector3f ChannelTable::digitizer_cm(int row) const
{
    return m_info->chs.at(static_cast<std::size_t>(row)).chpos.r0 * 100.0f;
}

//=============================================================================================================

bool ChannelTable::is_bad(int row) const
{
    return m_info->isBad(m_info->chs.at(static_cast<std::size_t>(row)).ch_name);
}

//=============================================================================================================

int ChannelTable::index_of_original(const std::string& name) const
{
    return channel_index(m_info->ch_names, name);
}

//=============================================================================================================

int ChannelTable::index_of_mapped(const std::string& name) const
{
    return index_of(m_mappedNames, name);
}

//=============================================================================================================

StringList ChannelTable::bad_channels() const
{
    return m_info->bads;
}

//=============================================================================================================

void ChannelTable::map_layout()
{
    m_aliases = m_info->ch_names;
    m_mappedNames = m_aliases;

    const std::vector<FiffChInfo>& chs = m_info->chs;
    for (std::size_t i = 0; i < chs.size() && i < m_mappedNames.size(); ++i) {
        switch (chs[i].kind) {
            case FIFFV_MEG_CH:
                // Remove the MEG prefix and separators, keep the residual channel number
                m_mappedNames[i] = "MEG " + layout_number(chs[i].ch_name, "MEG");
                break;
            case FIFFV_EEG_CH:
                m_mappedNames[i] = "EEG " + layout_number(chs[i].ch_name, "EEG");
                break;
            default:
                break;
        }
    }
}
