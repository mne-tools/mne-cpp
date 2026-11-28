//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     item_list.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ItemList: a list of texts with a selection, which hosts show as a list view without knowing its use.
 */

#ifndef VIZ_ITEM_LIST_H
#define VIZ_ITEM_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Toolkit-free model of a list view: its items, which of them are selected, and whether the user may
 * remove them.
 *
 * Panels hold item lists and hosts show them, e.g. viz::qt::ItemListView in Qt and FormView without a toolkit.
 * The selection follows the clicks of a list view: in Single mode a click selects one item; in Multi mode a click
 * selects one item and a click with @c extend (Ctrl held) toggles it. Removable lists drop their selected items,
 * except the protected ones, when the user presses Delete.
 *
 * @snippet ex_scene_sdl/main.cpp channel_selection_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ItemList
{
public:
    /** How items are selected. */
    enum class SelectionMode
    {
        None,   /**< Items are only shown. */
        Single, /**< One item at a time. */
        Multi   /**< Any number of items. */
    };

    /** What changed. */
    enum class Change
    {
        Items,    /**< The items, and with them the selection. */
        Selection /**< Only the selection. */
    };

    //=========================================================================================================
    /**
     * Constructs an empty list.
     *
     * @param[in] mode   How items are selected.
     */
    explicit ItemList(SelectionMode mode = SelectionMode::Single);

    //=========================================================================================================
    /**
     * @return How items are selected.
     */
    SelectionMode selection_mode() const
    {
        return m_mode;
    }

    //=========================================================================================================
    /**
     * Replaces the items; nothing is selected afterwards.
     *
     * @param[in] items   The items.
     */
    void set_items(std::vector<std::string> items);

    //=========================================================================================================
    /**
     * @return The items.
     */
    const std::vector<std::string>& items() const
    {
        return m_items;
    }

    //=========================================================================================================
    /**
     * @return The number of items.
     */
    int count() const
    {
        return static_cast<int>(m_items.size());
    }

    //=========================================================================================================
    /**
     * Adds an item at the end.
     *
     * @param[in] item   The item.
     */
    void append(std::string item);

    //=========================================================================================================
    /**
     * @param[in] item   An item.
     *
     * @return Its row, or -1.
     */
    int find(const std::string& item) const;

    //=========================================================================================================
    /**
     * Changes the selection as a click on @p row does; a row outside the list clears the selection.
     *
     * @param[in] row      Row clicked.
     * @param[in] extend   Toggle the row instead of selecting only it (Multi mode, Ctrl held).
     */
    void click(int row, bool extend = false);

    //=========================================================================================================
    /**
     * Selects exactly @p rows; rows outside the list are ignored, and Single mode keeps only the first.
     *
     * @param[in] rows   Rows to select.
     */
    void set_selected(std::vector<int> rows);

    //=========================================================================================================
    /**
     * @return The selected rows in increasing order.
     */
    const std::vector<int>& selected() const
    {
        return m_selected;
    }

    //=========================================================================================================
    /**
     * @param[in] row   A row.
     *
     * @return Whether it is selected.
     */
    bool is_selected(int row) const;

    //=========================================================================================================
    /**
     * @return The first selected row, or -1.
     */
    int current() const
    {
        return m_selected.empty() ? -1 : m_selected.front();
    }

    //=========================================================================================================
    /**
     * @return The first selected item, or an empty text.
     */
    std::string current_item() const;

    //=========================================================================================================
    /**
     * @param[in] removable   Whether the user may remove the selected items.
     */
    void set_removable(bool removable)
    {
        m_removable = removable;
    }

    //=========================================================================================================
    /**
     * @return Whether the user may remove the selected items.
     */
    bool removable() const
    {
        return m_removable;
    }

    //=========================================================================================================
    /**
     * @param[in] items   Items that are never removed, e.g. "All".
     */
    void set_protected_items(std::vector<std::string> items)
    {
        m_protected = std::move(items);
    }

    //=========================================================================================================
    /**
     * Removes the selected items but the protected ones, as Delete does in a removable list.
     *
     * @return Whether an item was removed; false for lists that are not removable.
     */
    bool remove_selected();

    //=========================================================================================================
    /**
     * @param[in] rows   Rows a host shows without scrolling.
     */
    void set_visible_rows(int rows)
    {
        m_visibleRows = rows > 0 ? rows : 1;
    }

    //=========================================================================================================
    /**
     * @return Rows a host shows without scrolling.
     */
    int visible_rows() const
    {
        return m_visibleRows;
    }

    //=========================================================================================================
    /**
     * @param[in] listener   Called after every change.
     *
     * @return An id for remove_listener().
     */
    int add_listener(std::function<void(Change)> listener);

    //=========================================================================================================
    /**
     * @param[in] id   Id from add_listener(); unknown ids are ignored.
     */
    void remove_listener(int id);

private:
    void notify(Change change) const;

    SelectionMode m_mode;                                   /**< How items are selected. */
    std::vector<std::string> m_items;                       /**< The items. */
    std::vector<int> m_selected;                            /**< Selected rows, increasing. */
    std::vector<std::string> m_protected;                   /**< Items never removed. */
    bool m_removable = false;                               /**< Whether the user may remove items. */
    int m_visibleRows = 6;                                  /**< Rows shown without scrolling. */
    std::map<int, std::function<void(Change)>> m_listeners; /**< Listeners by id. */
    int m_nextListener = 1;                                 /**< Id of the next listener. */
};

} // namespace mne::viz

#endif // VIZ_ITEM_LIST_H
