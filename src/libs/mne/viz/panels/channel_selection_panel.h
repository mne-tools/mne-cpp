//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_selection_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ChannelSelectionPanel picks channels in a 2-D sensor layout and keeps named channel groups.
 */

#ifndef VIZ_CHANNEL_SELECTION_PANEL_H
#define VIZ_CHANNEL_SELECTION_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../figures/sensor_layout_figure.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class ChannelTable;
class ItemList;

//=============================================================================================================
/**
 * @brief Tool panel to pick the channels the data views show: a sensor layout, named selection groups and the
 * channels picked by hand.
 *
 * The form chooses the layout file (.lout; EEG channels without one are laid out from their digitiser points), the
 * selection group file (.sel or .mon) and whether bad channels are drawn red, and saves the groups. The groups list
 * holds the groups of the file plus "All" (the channels mapped to the layout) and "All EEG"; choosing one shows its
 * channels and lets only them be picked in the layout. Clicks and rubber bands in the layout pick channels, which a
 * named group can keep. Delete removes picked channels and groups, but not "All" and "All EEG".
 *
 * The channels of the measurement come from a ChannelTable. The panel reports the channels to show (by their names
 * in the measurement), the picked sensors and every loaded layout through its callbacks.
 *
 * @snippet ex_scene_sdl/main.cpp channel_selection_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ChannelSelectionPanel : public Panel
{
public:
    /** Keys of the item lists. */
    static constexpr const char* groups_key = "groups";          /**< The selection groups. */
    static constexpr const char* channels_key = "groupChannels"; /**< The channels of the current group. */
    static constexpr const char* picked_key = "pickedChannels";  /**< The channels picked by hand. */

    /** What the panel reports to its host. */
    struct Callbacks
    {
        std::function<void(const std::vector<std::string>& channels)> channels_shown; /**< Names of the channels to show. */
        std::function<void(const std::vector<SensorLayout::Sensor>& sensors)>
            selection_changed;                                                         /**< The picked sensors, or all shown ones. */
        std::function<void(const std::map<std::string, Point>& layout)> layout_loaded; /**< A layout file was read. */
    };

    //=========================================================================================================
    /**
     * Constructs the panel with the Vectorview layout and the mne_browse_raw groups.
     *
     * @param[in] resources   Directory with the 2DLayouts and selectionGroups folders; empty for the installed
     *                        resources/general.
     */
    explicit ChannelSelectionPanel(std::filesystem::path resources = {});

    ~ChannelSelectionPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callbacks   What to call on changes.
     */
    void set_callbacks(Callbacks callbacks);

    //=========================================================================================================
    /**
     * @return The sensors of the layout.
     */
    SensorLayoutFigure& sensors() const
    {
        return *m_sensors;
    }

    //=========================================================================================================
    /**
     * @param[in] table   The channels of the measurement: names, layout names, kinds, digitiser points and bad
     *                    channels; null for none.
     */
    void set_channel_table(std::shared_ptr<const ChannelTable> table);

    //=========================================================================================================
    /**
     * Sets the channels of group "All", the measurement's channels that the layout knows, and selects the group.
     *
     * @param[in] names   Layout names of the channels.
     */
    void set_mapped_channels(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Reads a layout; EEG channels it does not have are laid out from their digitiser points.
     *
     * @param[in] file   A file of the 2DLayouts folder or an absolute path.
     *
     * @return Whether the file was read.
     */
    bool load_layout(const std::string& file);

    //=========================================================================================================
    /**
     * Reads the layout again, e.g. for a new measurement whose EEG channels have other digitiser points.
     */
    void reload_layout();

    //=========================================================================================================
    /**
     * Reads selection groups; "All" and "All EEG" are added.
     *
     * @param[in] file   A file of the selectionGroups folder or an absolute path, .sel or .mon.
     *
     * @return Whether the file was read.
     */
    bool load_selection_groups(const std::string& file);

    //=========================================================================================================
    /**
     * Writes the groups but "All" and "All EEG".
     *
     * @param[in] path   A .sel file, or a .mon file whose folder gets one file per group.
     *
     * @return Whether the groups were written.
     */
    bool save_selection_groups(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * @return The layout file shown.
     */
    std::string layout_file() const;

    //=========================================================================================================
    /**
     * @return The selection group file loaded.
     */
    std::string group_file() const;

    //=========================================================================================================
    /**
     * @return Layout position of each channel by layout name.
     */
    const std::map<std::string, Point>& layout_map() const
    {
        return m_layout;
    }

    //=========================================================================================================
    /**
     * @return The selection groups by name.
     */
    const std::map<std::string, std::vector<std::string>>& selection_groups() const
    {
        return m_groups;
    }

    //=========================================================================================================
    /**
     * @param[in] names   Layout names of the channels to ring in the layout.
     */
    void highlight_channels(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Picks channels as clicks in the layout do.
     *
     * @param[in] names   Layout names of the channels.
     */
    void select_channels(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * @return Layout names of the picked channels, or without any those of the current group.
     */
    std::vector<std::string> selected_channels() const;

    //=========================================================================================================
    /**
     * @return Whether group "All" is chosen and no channel is picked.
     */
    bool selection_empty() const;

    //=========================================================================================================
    /**
     * Draws the bad channels of the table red, if the form says so, and reports the channels to show.
     */
    void update_bad_channels();

    //=========================================================================================================
    /**
     * Reports the channels to show and the picked sensors.
     */
    void update_data_view();

    //=========================================================================================================
    /**
     * Removes the sensors.
     */
    void clear();

    //=========================================================================================================
    /**
     * @return The selection groups.
     */
    ItemList& groups() const
    {
        return *m_groupList;
    }

    //=========================================================================================================
    /**
     * @return The channels of the current group.
     */
    ItemList& group_channels() const
    {
        return *m_channelList;
    }

    //=========================================================================================================
    /**
     * @return The channels picked by hand.
     */
    ItemList& picked_channels() const
    {
        return *m_pickedList;
    }

private:
    std::filesystem::path resolve(const std::filesystem::path& folder, const std::string& file) const;
    void changed(const std::string& key);
    void show_groups();
    void group_chosen();
    void groups_removed();
    void sensors_picked();
    void add_group();
    void set_field(const std::string& key, const std::string& value);
    std::vector<std::string> eeg_channels() const;

    std::filesystem::path m_layouts;                          /**< The 2DLayouts folder. */
    std::filesystem::path m_selections;                       /**< The selectionGroups folder. */
    std::shared_ptr<SensorLayoutFigure> m_sensors;            /**< The sensors. */
    std::shared_ptr<ItemList> m_groupList;                    /**< The selection groups. */
    std::shared_ptr<ItemList> m_channelList;                  /**< The channels of the current group. */
    std::shared_ptr<ItemList> m_pickedList;                   /**< The channels picked by hand. */
    std::shared_ptr<const ChannelTable> m_table;              /**< The channels of the measurement. */
    std::map<std::string, Point> m_layout;                    /**< Layout position by layout name. */
    std::map<std::string, std::vector<std::string>> m_groups; /**< Channels by group name. */
    std::vector<std::string> m_mapped;                        /**< Channels of group "All". */
    Callbacks m_callbacks;                                    /**< What to call on changes. */
    std::vector<std::pair<ItemList*, int>> m_listeners;       /**< Listeners on the lists. */
    int m_formListener = 0;                                   /**< Listener on the form. */
    bool m_updating = false;                                  /**< Whether the panel changes its own state. */
};

} // namespace mne::viz

#endif // VIZ_CHANNEL_SELECTION_PANEL_H
