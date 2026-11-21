//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     selectionio.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2014
 * @brief    Reader / writer for MNE @c .sel channel-selection files and Brainstorm @c .mon montage files.
 *
 * @ref mne::SelectionIO converts between two text-based
 * grouping formats that mne-cpp's GUI tools and command-line
 * pipelines hand back and forth with sibling toolchains:
 *
 * - MNE @c .sel — one selection group per line, holding the
 *   channels visible in a given montage ("Left-temporal",
 *   "All MEG", ...); shared with mne-python and mne-c.
 *
 * - Brainstorm @c .mon — one montage definition per file,
 *   used by the Brainstorm GUI for the same purpose.
 *
 * Groups keep their file order through a read/write round trip.
 */

#ifndef CHANNELS_SELECTIONIO_H
#define CHANNELS_SELECTIONIO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channels_global.h"
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A named group of channels.
 *
 * @snippet ex_channels/main.cpp selection_io_usage
 */
struct CHANNELSSHARED_EXPORT ChannelSelection
{
    std::string name;         /**< Group name, e.g. "Left-temporal". */
    mne::StringList channels; /**< Channel names, in file order. */

    bool operator==(const ChannelSelection&) const = default;
};

//=============================================================================================================
/**
 * Reads and writes channel selection groups.
 *
 * @brief Reader/writer for MNE .sel channel-selection files and Brainstorm .mon montage files.
 *
 * @snippet ex_channels/main.cpp selection_io_usage
 */
class CHANNELSSHARED_EXPORT SelectionIO
{
public:
    //=========================================================================================================
    /**
     * Reads an MNE .sel file: one @c "name:ch1|ch2|..." group per line, @c % lines are comments.
     *
     * @param[in] path  The .sel file.
     *
     * @return The groups in file order, or no value if the path has no .sel extension or cannot be opened.
     */
    static std::optional<std::vector<ChannelSelection>> read_sel(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads a Brainstorm .mon file: the group name, then one @c "channel : expression" line per channel.
     *
     * @param[in] path  The .mon file.
     *
     * @return The group, or no value if the path has no .mon extension or cannot be opened.
     */
    static std::optional<ChannelSelection> read_mon(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes groups to an MNE .sel file.
     *
     * @param[in] path    The .sel file.
     * @param[in] groups  The groups.
     *
     * @return Whether the file was written; false if the path has no .sel extension.
     */
    static bool write_sel(const std::filesystem::path& path, std::span<const ChannelSelection> groups);

    //=========================================================================================================
    /**
     * Writes each group to its own Brainstorm .mon file, named after the group.
     *
     * @param[in] directory  The directory to write to.
     * @param[in] groups     The groups.
     *
     * @return Whether every file was written.
     */
    static bool write_mon(const std::filesystem::path& directory, std::span<const ChannelSelection> groups);
};

} // namespace mne

#endif // CHANNELS_SELECTIONIO_H
