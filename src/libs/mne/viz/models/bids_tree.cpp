//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_tree.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of BidsTree.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_tree.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <stdexcept>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

std::string with_prefix(std::string_view name, std::string_view prefix)
{
    std::string result = name.starts_with(prefix) ? std::string(name) : std::string(prefix) + std::string(name);
    std::erase(result, ' ');
    return result;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::string BidsTree::subject_name(std::string_view name)
{
    return with_prefix(name, "sub-");
}

//=============================================================================================================

std::string BidsTree::session_name(std::string_view name)
{
    return with_prefix(name, "ses-");
}

//=============================================================================================================

std::string BidsTree::folder_name(int type)
{
    switch (type) {
        case Functional:
            return "func";
        case Anatomical:
            return "anat";
        case Behavioral:
            return "beh";
        default:
            return "unknown";
    }
}

//=============================================================================================================

int BidsTree::add_subject(std::string_view name)
{
    const int id = create(subject_name(name), Subject, -1);
    m_nodes[static_cast<std::size_t>(id)].subject = id;
    return id;
}

//=============================================================================================================

std::vector<int> BidsTree::add_session(std::string_view subject, std::string_view session)
{
    std::vector<int> matches;
    for (int id : m_subjects) {
        if (node(id).name == subject) {
            matches.push_back(id);
        }
    }

    std::vector<int> sessions;
    sessions.reserve(matches.size());
    for (std::size_t i = 0; i < matches.size(); ++i) {
        if (matches.size() > 1) {
            m_nodes[static_cast<std::size_t>(matches[i])].name += std::to_string(i + 1);
        }
        sessions.push_back(add_session(matches[i], session));
    }
    return sessions;
}

//=============================================================================================================

int BidsTree::add_session(int subject, std::string_view session)
{
    const int id = create(session_name(session), Session, subject);
    set_owner(id, subject, id);
    return id;
}

//=============================================================================================================

BidsTree::Placement BidsTree::add_data_to_session(int session, std::string_view name, int type)
{
    Placement placement;
    const std::string folderName = folder_name(type);
    for (int child : node(session).children) {
        if (node(child).name == folderName) {
            placement.folder = child;
            break;
        }
    }

    if (placement.folder < 0) {
        placement.folder = create(folderName, Folder, session);
        placement.folder_created = true;
        set_owner(placement.folder, node(session).subject, session);
    }

    placement.item = create(std::string(name), type, placement.folder);
    set_owner(placement.item, node(session).subject, session);
    return placement;
}

//=============================================================================================================

int BidsTree::add_to_data(int parent, std::string_view name, int type)
{
    const int id = create(std::string(name), type, parent);
    set_owner(id, node(parent).subject, node(parent).session);
    return id;
}

//=============================================================================================================

void BidsTree::move_session(int session, int subject)
{
    std::erase(siblings(session), session);
    m_nodes[static_cast<std::size_t>(session)].parent = subject;
    m_nodes[static_cast<std::size_t>(subject)].children.push_back(session);
    set_owner(session, subject, session);
}

//=============================================================================================================

BidsTree::Placement BidsTree::move_data(int item, int session)
{
    const int folder = node(item).parent;
    std::erase(siblings(item), item);
    if (folder >= 0 && node(folder).type == Folder && node(folder).children.empty()) {
        remove(folder);
    }

    const std::string name = node(item).name;
    Placement placement = add_data_to_session(session, name, node(item).type);

    // Put the moved item where the placeholder went, so it keeps its id and sub-items.
    const int placeholder = placement.item;
    std::replace(m_nodes[static_cast<std::size_t>(placement.folder)].children.begin(),
                 m_nodes[static_cast<std::size_t>(placement.folder)].children.end(), placeholder, item);
    m_alive[static_cast<std::size_t>(placeholder)] = false;
    m_nodes[static_cast<std::size_t>(item)].parent = placement.folder;
    set_owner(item, node(session).subject, session);

    placement.item = item;
    return placement;
}

//=============================================================================================================

void BidsTree::remove(int id)
{
    if (!contains(id)) {
        return;
    }
    std::erase(siblings(id), id);
    erase_subtree(id);
}

//=============================================================================================================

void BidsTree::rename(int id, std::string name)
{
    if (contains(id)) {
        m_nodes[static_cast<std::size_t>(id)].name = std::move(name);
    }
}

//=============================================================================================================

bool BidsTree::contains(int id) const
{
    return id >= 0 && id < std::ssize(m_nodes) && m_alive[static_cast<std::size_t>(id)];
}

//=============================================================================================================

const BidsTree::Node& BidsTree::node(int id) const
{
    if (!contains(id)) {
        throw std::out_of_range("BidsTree: no node " + std::to_string(id));
    }
    return m_nodes[static_cast<std::size_t>(id)];
}

//=============================================================================================================

std::vector<int> BidsTree::path(int id) const
{
    std::vector<int> rows;
    for (int current = id; current >= 0; current = node(current).parent) {
        const int parent = node(current).parent;
        const std::vector<int>& level = parent < 0 ? m_subjects : node(parent).children;
        rows.push_back(static_cast<int>(std::ranges::find(level, current) - level.begin()));
    }
    std::ranges::reverse(rows);
    return rows;
}

//=============================================================================================================

int BidsTree::create(std::string name, int type, int parent)
{
    const int id = static_cast<int>(m_nodes.size());
    Node created;
    created.name = std::move(name);
    created.type = type;
    created.parent = parent;
    m_nodes.push_back(std::move(created));
    m_alive.push_back(true);

    if (parent < 0) {
        m_subjects.push_back(id);
    } else {
        m_nodes[static_cast<std::size_t>(parent)].children.push_back(id);
    }
    return id;
}

//=============================================================================================================

std::vector<int>& BidsTree::siblings(int id)
{
    const int parent = node(id).parent;
    return parent < 0 ? m_subjects : m_nodes[static_cast<std::size_t>(parent)].children;
}

//=============================================================================================================

void BidsTree::set_owner(int id, int subject, int session)
{
    Node& current = m_nodes[static_cast<std::size_t>(id)];
    current.subject = subject;
    current.session = session;
    for (int child : current.children) {
        set_owner(child, subject, session);
    }
}

//=============================================================================================================

void BidsTree::erase_subtree(int id)
{
    for (int child : m_nodes[static_cast<std::size_t>(id)].children) {
        erase_subtree(child);
    }
    m_nodes[static_cast<std::size_t>(id)].children.clear();
    m_alive[static_cast<std::size_t>(id)] = false;
}
