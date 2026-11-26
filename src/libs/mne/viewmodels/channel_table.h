//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_table.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ChannelTable is the view model of a channel information table: one row per channel, 13 columns.
 *
 * The rows are the channels of the measurement info; the columns show the channel number, original name, alias,
 * the name the channel has in a 2-D sensor layout, kind, MEG type, unit, layout position, digitizer position,
 * active filter, coil type, bad flag and number of compensators. ChannelTable maps MEG and EEG channel names to
 * layout names ("MEG 0113", "EEG 001") and finds rows by name, without a GUI toolkit; disp::ChannelInfoModel
 * adapts it to a Qt item model.
 */

#ifndef VIEWMODELS_CHANNEL_TABLE_H
#define VIEWMODELS_CHANNEL_TABLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewmodels_global.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viewmodels
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

namespace mne::viewmodels
{

//=============================================================================================================
/**
 * @brief Channel rows and their presentation attributes for a channel information table.
 *
 * @since 3.0.0
 */
class VIEWMODELSSHARED_EXPORT ChannelTable
{
public:
    //=========================================================================================================
    /**
     * @brief Columns of the table.
     */
    enum class Column
    {
        Number,           /**< Row of the channel in the data matrix. */
        OriginalName,     /**< Channel name in the measurement info. */
        Alias,            /**< Alias name. */
        MappedLayoutName, /**< Name in the 2-D sensor layout. */
        Kind,             /**< FIFF channel kind. */
        MegType,          /**< MEG_grad, MEG_mag or non_MEG. */
        Unit,             /**< FIFF unit. */
        Position,         /**< Position in the 2-D sensor layout. */
        Digitizer,        /**< Coil origin in cm. */
        ActiveFilter,     /**< Active filter, currently always empty. */
        CoilType,         /**< FIFF coil type. */
        Bad,              /**< Whether the channel is bad. */
        Compensators      /**< Number of compensators of the info. */
    };

    static constexpr int column_count = 13; /**< Number of columns. */

    //=========================================================================================================
    /**
     * Creates an empty table.
     *
     * @since 3.0.0
     */
    ChannelTable();

    //=========================================================================================================
    /**
     * @param[in] column    A column.
     *
     * @return The header text of @p column.
     *
     * @since 3.0.0
     */
    static std::string header(Column column);

    //=========================================================================================================
    /**
     * Sets the measurement info; aliases start as the channel names and are mapped to layout names.
     *
     * @param[in] info  Measurement info, shared with the caller.
     *
     * @since 3.0.0
     */
    void set_info(std::shared_ptr<FiffInfo> info);

    //=========================================================================================================
    /**
     * @return The measurement info.
     *
     * @since 3.0.0
     */
    const std::shared_ptr<FiffInfo>& info() const
    {
        return m_info;
    }

    //=========================================================================================================
    /**
     * Sets the 2-D sensor layout and maps the channels to it again.
     *
     * @param[in] layout    Position of each layout channel name.
     *
     * @since 3.0.0
     */
    void set_layout(std::map<std::string, std::array<double, 2>> layout);

    //=========================================================================================================
    /**
     * Drops the info, the layout and all names.
     *
     * @since 3.0.0
     */
    void clear();

    //=========================================================================================================
    /**
     * @return Number of channels.
     *
     * @since 3.0.0
     */
    int row_count() const;

    //=========================================================================================================
    /**
     * @param[in] row       Row of a channel.
     * @param[in] column    A column.
     *
     * @return The display text of the cell, empty for a row outside the table.
     *
     * @since 3.0.0
     */
    std::string text(int row, Column column) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of a channel.
     *
     * @return "MEG_grad", "MEG_mag" or "non_MEG".
     *
     * @since 3.0.0
     */
    std::string meg_type(int row) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of a channel.
     *
     * @return Position of the channel's layout name, (0, 0) when the layout lacks it.
     *
     * @since 3.0.0
     */
    std::array<double, 2> position(int row) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of a channel.
     *
     * @return Coil origin in cm.
     *
     * @since 3.0.0
     */
    Eigen::Vector3f digitizer_cm(int row) const;

    //=========================================================================================================
    /**
     * @param[in] row   Row of a channel.
     *
     * @return Whether the channel is bad.
     *
     * @since 3.0.0
     */
    bool is_bad(int row) const;

    //=========================================================================================================
    /**
     * @return The alias names.
     *
     * @since 3.0.0
     */
    const std::vector<std::string>& aliases() const
    {
        return m_aliases;
    }

    //=========================================================================================================
    /**
     * @return The layout names of all channels.
     *
     * @since 3.0.0
     */
    const std::vector<std::string>& mapped_names() const
    {
        return m_mappedNames;
    }

    //=========================================================================================================
    /**
     * @param[in] name  Original channel name.
     *
     * @return Row of the channel, or -1.
     *
     * @since 3.0.0
     */
    int index_of_original(const std::string& name) const;

    //=========================================================================================================
    /**
     * @param[in] name  Layout channel name.
     *
     * @return Row of the channel, or -1.
     *
     * @since 3.0.0
     */
    int index_of_mapped(const std::string& name) const;

    //=========================================================================================================
    /**
     * @return The bad channels of the info.
     *
     * @since 3.0.0
     */
    StringList bad_channels() const;

private:
    void map_layout();

    std::shared_ptr<FiffInfo> m_info;
    std::map<std::string, std::array<double, 2>> m_layout;
    std::vector<std::string> m_aliases;
    std::vector<std::string> m_mappedNames;
};

} // namespace mne::viewmodels

#endif // VIEWMODELS_CHANNEL_TABLE_H
