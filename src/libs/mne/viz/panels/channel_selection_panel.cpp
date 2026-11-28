//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_selection_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ChannelSelectionPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_selection_panel.h"
#include "../models/channel_table.h"
#include "../models/item_list.h"
#include <mne/base/string_utils.h>
#include <mne/channels/layoutmaker.h>
#include <mne/channels/selectionio.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/mne/mne_layout.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Field keys; the files keep the QSettings keys of the former ChannelSelectionView
constexpr const char* kLayoutFile = "selectedLayoutFile";
constexpr const char* kGroupFile = "selectedGroupFile";
constexpr const char* kShowBad = "showBadChannelsAsRed";
constexpr const char* kSaveFile = "saveGroupFile";
constexpr const char* kGroupName = "groupName";
constexpr const char* kAddGroup = "addGroup";

constexpr const char* kAll = "All";
constexpr const char* kAllEeg = "All EEG";
constexpr const char* kDefaultLayout = "Vectorview-all.lout";
constexpr const char* kDefaultGroups = "mne_browse_raw_vv.sel";
constexpr std::array<const char*, 6> kLayouts = {"babymeg-mag-inner-layer.lout", "babymeg-mag-outer-layer.lout", "Vectorview-grad.lout",
                                                 "Vectorview-all.lout", "Vectorview-mag.lout", "standard_waveguard64_duke.lout"};

constexpr const char* kLayoutGroup = "Layout";
constexpr const char* kGroupsGroup = "Selection groups";
constexpr const char* kChannelsGroup = "Channels";

std::vector<SettingChoice> layout_choices()
{
    std::vector<SettingChoice> choices;
    choices.reserve(kLayouts.size());
    for (const char* file : kLayouts) {
        choices.push_back({file, file});
    }
    return choices;
}

bool has_extension(const std::filesystem::path& path, const char* extension)
{
    return mne::utf8_from_path(path).find(extension) != std::string::npos;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSelectionPanel::ChannelSelectionPanel(std::filesystem::path resources)
: Panel("ChannelSelectionView", "Channel Selection")
, m_sensors(std::make_shared<SensorLayoutFigure>())
, m_groupList(std::make_shared<ItemList>(ItemList::SelectionMode::Single))
, m_channelList(std::make_shared<ItemList>(ItemList::SelectionMode::None))
, m_pickedList(std::make_shared<ItemList>(ItemList::SelectionMode::Multi))
{
    if (resources.empty()) {
        resources = mne::resource_path("general");
    }
    m_layouts = resources / "2DLayouts";
    m_selections = resources / "selectionGroups";
    set_figure(m_sensors);

    SettingsForm& f = form();
    f.add_choice(kLayoutFile, "Layout", layout_choices(), kDefaultLayout, kLayoutGroup);
    f.add_bool(kShowBad, "Show bad channels in red", true, kLayoutGroup);
    f.add_path(kGroupFile, "Selection groups", kDefaultGroups, "Selection files (*.sel *.mon)", kLayoutGroup);
    SettingField save;
    save.key = kSaveFile;
    save.kind = SettingField::Kind::Path;
    save.label = "Save groups to";
    save.default_value = std::string();
    save.file_filter = "MNE selection file (*.sel);;Brainstorm montage file (*.mon)";
    save.save_path = true;
    save.persistent = false;
    save.group = kLayoutGroup;
    f.add(std::move(save));

    m_groupList->set_removable(true);
    m_groupList->set_protected_items({kAll, kAllEeg});
    m_pickedList->set_removable(true);
    add_list(groups_key, "Groups", m_groupList, kGroupsGroup);
    SettingField name;
    name.key = kGroupName;
    name.kind = SettingField::Kind::Text;
    name.label = "Name of a new group";
    name.default_value = std::string();
    name.persistent = false;
    name.group = kGroupsGroup;
    f.add(std::move(name));
    f.add_action(kAddGroup, "Add picked channels as group", kGroupsGroup);
    add_list(channels_key, "Channels of the group", m_channelList, kChannelsGroup);
    add_list(picked_key, "Picked channels", m_pickedList, kChannelsGroup);

    m_sensors->set_selection_callback([this]() {
        sensors_picked();
    });
    m_listeners.emplace_back(m_groupList.get(), m_groupList->add_listener([this](ItemList::Change change) {
        if (m_updating) {
            return;
        }
        if (change == ItemList::Change::Selection) {
            group_chosen();
        } else {
            groups_removed();
        }
    }));
    m_listeners.emplace_back(m_pickedList.get(), m_pickedList->add_listener([this](ItemList::Change change) {
        // Channels the user removed are no longer picked in the layout either
        if (m_updating || change != ItemList::Change::Items) {
            return;
        }
        const bool updating = std::exchange(m_updating, true);
        m_sensors->select_names(m_pickedList->items());
        m_updating = updating;
        update_data_view();
    }));
    m_formListener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kAddGroup)) {
            changed(key);
        }
    });

    load_layout(kDefaultLayout);
    load_selection_groups(kDefaultGroups);
}

//=============================================================================================================

ChannelSelectionPanel::~ChannelSelectionPanel()
{
    form().remove_listener(m_formListener);
    for (const auto& [list, id] : m_listeners) {
        list->remove_listener(id);
    }
    m_sensors->set_selection_callback({});
}

//=============================================================================================================

void ChannelSelectionPanel::set_callbacks(Callbacks callbacks)
{
    m_callbacks = std::move(callbacks);
}

//=============================================================================================================

void ChannelSelectionPanel::set_channel_table(std::shared_ptr<const ChannelTable> table)
{
    m_table = std::move(table);
}

//=============================================================================================================

void ChannelSelectionPanel::set_mapped_channels(const std::vector<std::string>& names)
{
    m_mapped = names;
    show_groups();
}

//=============================================================================================================

bool ChannelSelectionPanel::load_layout(const std::string& file)
{
    set_field(kLayoutFile, file);
    const std::optional<mne::MNELayout> layout = file.empty() ? std::nullopt : mne::MNELayout::read(resolve(m_layouts, file));
    m_layout.clear();
    if (layout) {
        for (const auto& [name, position] : layout->channelPositions()) {
            m_layout.insert_or_assign(name, Point{position.x(), position.y()});
        }
    }

    // EEG channels the file does not have are laid out from their digitiser points
    std::vector<Eigen::Vector3d> positions;
    std::vector<std::string> names;
    const int rows = m_table ? m_table->row_count() : 0;
    for (int row = 0; row < rows; ++row) {
        if (m_table->info()->chs[static_cast<std::size_t>(row)].kind == FIFFV_EEG_CH) {
            positions.push_back(m_table->digitizer_cm(row).cast<double>());
            names.push_back(m_table->text(row, ChannelTable::Column::OriginalName));
        }
    }
    if (!positions.empty()) {
        if (const auto corners = mne::LayoutMaker::make_layout(positions, {.radius = 60.0, .box_width = 5.0, .box_height = 4.0, .mirror_y = true})) {
            for (std::size_t i = 0; i < corners->size() && i < names.size(); ++i) {
                m_layout.try_emplace(names[i], Point{(*corners)[i].x(), (*corners)[i].y()});
            }
        }
    }

    update_bad_channels();
    if (layout && m_callbacks.layout_loaded) {
        m_callbacks.layout_loaded(m_layout);
    }
    return layout.has_value();
}

//=============================================================================================================

void ChannelSelectionPanel::reload_layout()
{
    load_layout(layout_file());
}

//=============================================================================================================

bool ChannelSelectionPanel::load_selection_groups(const std::string& file)
{
    set_field(kGroupFile, file);
    const std::filesystem::path path = resolve(m_selections, file);
    std::optional<std::vector<mne::ChannelSelection>> read;
    if (!file.empty()) {
        if (has_extension(path, ".sel")) {
            read = mne::SelectionIO::read_sel(path);
        }
        if (has_extension(path, ".mon")) {
            if (auto group = mne::SelectionIO::read_mon(path)) {
                read = std::vector<mne::ChannelSelection>{std::move(*group)};
            }
        }
    }
    m_groups.clear();
    if (read) {
        for (mne::ChannelSelection& group : *read) {
            m_groups.insert_or_assign(group.name, std::move(group.channels));
        }
    }
    show_groups();
    return read.has_value();
}

//=============================================================================================================

bool ChannelSelectionPanel::save_selection_groups(const std::filesystem::path& path) const
{
    std::vector<mne::ChannelSelection> groups;
    for (const auto& [name, channels] : m_groups) {
        if (name != kAll && name != kAllEeg) {
            groups.push_back(mne::ChannelSelection{name, channels});
        }
    }
    if (has_extension(path, ".sel")) {
        return mne::SelectionIO::write_sel(path, groups);
    }
    if (has_extension(path, ".mon")) {
        return mne::SelectionIO::write_mon(path.parent_path(), groups);
    }
    return false;
}

//=============================================================================================================

std::string ChannelSelectionPanel::layout_file() const
{
    return form().get<std::string>(kLayoutFile);
}

//=============================================================================================================

std::string ChannelSelectionPanel::group_file() const
{
    return form().get<std::string>(kGroupFile);
}

//=============================================================================================================

void ChannelSelectionPanel::highlight_channels(const std::vector<std::string>& names)
{
    m_sensors->highlight_names(names);
}

//=============================================================================================================

void ChannelSelectionPanel::select_channels(const std::vector<std::string>& names)
{
    m_sensors->select_names(names);
}

//=============================================================================================================

std::vector<std::string> ChannelSelectionPanel::selected_channels() const
{
    return m_pickedList->count() > 0 ? m_pickedList->items() : m_channelList->items();
}

//=============================================================================================================

bool ChannelSelectionPanel::selection_empty() const
{
    return m_groupList->current_item() == kAll && m_pickedList->count() == 0;
}

//=============================================================================================================

void ChannelSelectionPanel::update_bad_channels()
{
    std::vector<std::string> bad;
    if (m_table && form().get<bool>(kShowBad)) {
        const StringList badNames = m_table->bad_channels();
        for (int row = 0; row < m_table->row_count(); ++row) {
            if (std::ranges::contains(badNames, m_table->text(row, ChannelTable::Column::OriginalName))) {
                bad.push_back(m_table->text(row, ChannelTable::Column::MappedLayoutName));
            }
        }
    }
    m_sensors->set_layout(m_layout, bad);
    m_sensors->set_selectable(m_channelList->items());
    update_data_view();
}

//=============================================================================================================

void ChannelSelectionPanel::update_data_view()
{
    if (m_callbacks.channels_shown) {
        // By the names of the measurement where the table knows the layout names
        std::vector<std::string> names;
        for (const std::string& name : selected_channels()) {
            const int row = m_table ? m_table->index_of_mapped(name) : -1;
            names.push_back(row >= 0 ? m_table->text(row, ChannelTable::Column::OriginalName) : name);
        }
        m_callbacks.channels_shown(names);
    }
    if (m_callbacks.selection_changed) {
        m_callbacks.selection_changed(m_sensors->selection());
    }
}

//=============================================================================================================

void ChannelSelectionPanel::clear()
{
    m_sensors->clear();
}

//=============================================================================================================

std::filesystem::path ChannelSelectionPanel::resolve(const std::filesystem::path& folder, const std::string& file) const
{
    const std::filesystem::path path = mne::path_from_utf8(file);
    return path.is_absolute() ? path : folder / path;
}

//=============================================================================================================

void ChannelSelectionPanel::changed(const std::string& key)
{
    if (key == kLayoutFile) {
        load_layout(layout_file());
    } else if (key == kShowBad) {
        update_bad_channels();
    } else if (key == kGroupFile) {
        load_selection_groups(group_file());
    } else if (key == kSaveFile) {
        if (const auto path = form().get<std::string>(kSaveFile); !path.empty()) {
            save_selection_groups(mne::path_from_utf8(path));
        }
    } else if (key == kAddGroup) {
        add_group();
    }
}

//=============================================================================================================

void ChannelSelectionPanel::show_groups()
{
    // "All" depends on the measurement and "All EEG" on its channels, not on the group file
    m_groups.insert_or_assign(kAll, m_mapped);
    m_groups.insert_or_assign(kAllEeg, eeg_channels());
    std::vector<std::string> names;
    names.reserve(m_groups.size());
    for (const auto& [name, channels] : m_groups) {
        names.push_back(name);
    }
    const bool updating = std::exchange(m_updating, true);
    m_groupList->set_items(std::move(names));
    m_updating = updating;
    // The list has no selection after set_items(), so choosing "All" always shows its channels
    m_groupList->set_selected({m_groupList->find(kAll)});
}

//=============================================================================================================

void ChannelSelectionPanel::group_chosen()
{
    const std::string name = m_groupList->current_item();
    const auto group = m_groups.find(name);
    if (group == m_groups.end()) {
        return;
    }
    m_sensors->set_channel_kind(name.find("EEG") != std::string::npos ? FIFFV_EEG_CH : FIFFV_MEG_CH);
    m_channelList->set_items(group->second);
    update_bad_channels();
}

//=============================================================================================================

void ChannelSelectionPanel::groups_removed()
{
    std::erase_if(m_groups, [this](const auto& group) {
        return m_groupList->find(group.first) < 0;
    });
    update_data_view();
}

//=============================================================================================================

void ChannelSelectionPanel::sensors_picked()
{
    if (m_updating) {
        return;
    }
    const bool updating = std::exchange(m_updating, true);
    m_pickedList->set_items(m_sensors->selected_names());
    m_updating = updating;
    update_data_view();
}

//=============================================================================================================

void ChannelSelectionPanel::add_group()
{
    const std::string name = form().get<std::string>(kGroupName);
    if (name.empty()) {
        return;
    }
    const std::string current = m_groupList->current_item();
    m_groups.insert_or_assign(name, m_pickedList->items());
    std::vector<std::string> names;
    names.reserve(m_groups.size());
    for (const auto& [group, channels] : m_groups) {
        names.push_back(group);
    }
    const bool updating = std::exchange(m_updating, true);
    m_groupList->set_items(std::move(names));
    m_groupList->set_selected({m_groupList->find(current)});
    m_updating = updating;
}

//=============================================================================================================

void ChannelSelectionPanel::set_field(const std::string& key, const std::string& value)
{
    const bool updating = std::exchange(m_updating, true);
    if (key == kLayoutFile) {
        if (const SettingField* field = form().field(key); field && std::ranges::none_of(field->choices, [&value](const SettingChoice& choice) {
                                                               return choice.value == value;
                                                           })) {
            std::vector<SettingChoice> choices = field->choices;
            choices.push_back({value, mne::utf8_from_path(mne::path_from_utf8(value).filename())});
            form().set_choices(key, std::move(choices));
        }
    }
    form().set_value(key, value);
    m_updating = updating;
}

//=============================================================================================================

std::vector<std::string> ChannelSelectionPanel::eeg_channels() const
{
    std::vector<std::string> names;
    const int rows = m_table ? m_table->row_count() : 0;
    for (int row = 0; row < rows; ++row) {
        if (m_table->info()->chs[static_cast<std::size_t>(row)].kind == FIFFV_EEG_CH) {
            names.push_back(m_table->text(row, ChannelTable::Column::OriginalName));
        }
    }
    return names;
}
