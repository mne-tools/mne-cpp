//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     item_list.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ItemList class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "item_list.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ItemList::ItemList(SelectionMode mode)
: m_mode(mode)
{
}

//=============================================================================================================

void ItemList::set_items(std::vector<std::string> items)
{
    m_items = std::move(items);
    m_selected.clear();
    notify(Change::Items);
}

//=============================================================================================================

void ItemList::append(std::string item)
{
    m_items.push_back(std::move(item));
    notify(Change::Items);
}

//=============================================================================================================

int ItemList::find(const std::string& item) const
{
    const auto it = std::ranges::find(m_items, item);
    return it == m_items.end() ? -1 : static_cast<int>(it - m_items.begin());
}

//=============================================================================================================

void ItemList::click(int row, bool extend)
{
    if (m_mode == SelectionMode::None) {
        return;
    }
    std::vector<int> rows;
    if (row >= 0 && row < count()) {
        if (extend && m_mode == SelectionMode::Multi) {
            rows = m_selected;
            if (const auto it = std::ranges::find(rows, row); it != rows.end()) {
                rows.erase(it);
            } else {
                rows.push_back(row);
            }
        } else {
            rows = {row};
        }
    }
    set_selected(std::move(rows));
}

//=============================================================================================================

void ItemList::set_selected(std::vector<int> rows)
{
    std::erase_if(rows, [this](int row) { return row < 0 || row >= count(); });
    std::ranges::sort(rows);
    const auto [first, last] = std::ranges::unique(rows);
    rows.erase(first, last);
    if (m_mode == SelectionMode::None) {
        rows.clear();
    } else if (m_mode == SelectionMode::Single && rows.size() > 1) {
        rows.resize(1);
    }
    if (rows != m_selected) {
        m_selected = std::move(rows);
        notify(Change::Selection);
    }
}

//=============================================================================================================

bool ItemList::is_selected(int row) const
{
    return std::ranges::binary_search(m_selected, row);
}

//=============================================================================================================

std::string ItemList::current_item() const
{
    return m_selected.empty() ? std::string() : m_items[static_cast<std::size_t>(m_selected.front())];
}

//=============================================================================================================

bool ItemList::remove_selected()
{
    if (!m_removable) {
        return false;
    }
    std::vector<std::string> kept;
    kept.reserve(m_items.size());
    for (int row = 0; row < count(); ++row) {
        const std::string& item = m_items[static_cast<std::size_t>(row)];
        if (!is_selected(row) || std::ranges::contains(m_protected, item)) {
            kept.push_back(item);
        }
    }
    if (kept.size() == m_items.size()) {
        return false;
    }
    set_items(std::move(kept));
    return true;
}

//=============================================================================================================

int ItemList::add_listener(std::function<void(Change)> listener)
{
    const int id = m_nextListener++;
    m_listeners.emplace(id, std::move(listener));
    return id;
}

//=============================================================================================================

void ItemList::remove_listener(int id)
{
    m_listeners.erase(id);
}

//=============================================================================================================

void ItemList::notify(Change change) const
{
    // A listener may remove itself or others
    const auto listeners = m_listeners;
    for (const auto& [id, listener] : listeners) {
        if (listener) {
            listener(change);
        }
    }
}
