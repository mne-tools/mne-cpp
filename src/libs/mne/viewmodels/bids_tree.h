//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_tree.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BidsTree is the view model of a BIDS browser: subjects, sessions, data-type folders and data items.
 *
 * A BIDS browser shows a tree of subjects ("sub-01"), their sessions ("ses-01"), a folder per data type ("func",
 * "anat", "beh") and the data items in it, which can carry sub-items such as averages, events or dipoles.
 * BidsTree holds that structure and its rules without a GUI toolkit: BIDS prefixes, folder per data type,
 * renaming of subjects with the same name, and removing a folder when its last item moves away.
 * disp::BidsViewModel mirrors it in a Qt item model.
 */

#ifndef VIEWMODELS_BIDS_TREE_H
#define VIEWMODELS_BIDS_TREE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewmodels_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viewmodels
//=============================================================================================================

namespace mne::viewmodels
{

//=============================================================================================================
/**
 * @brief Subjects, sessions, data-type folders and data items of a BIDS browser.
 *
 * Nodes are addressed by ids that stay valid until the node is removed. The item types use the values of the
 * BIDS_* constants of disp::BidsViewModel (subject 1, session 2, folder 3, functional 10, anatomical 11,
 * behavioral 12, iEEG 13, average 20, event 21, dipole 22, electrodes 23, coordinate system 24, unknown 99).
 *
 * @since 3.0.0
 */
class VIEWMODELSSHARED_EXPORT BidsTree
{
public:
    static constexpr int Subject = 1;     /**< Subject node. */
    static constexpr int Session = 2;     /**< Session node. */
    static constexpr int Folder = 3;      /**< Data-type folder. */
    static constexpr int Functional = 10; /**< Functional data, e.g. a raw recording. */
    static constexpr int Anatomical = 11; /**< Anatomical data. */
    static constexpr int Behavioral = 12; /**< Behavioral data. */
    static constexpr int Unknown = 99;    /**< Data of unknown type. */

    //=========================================================================================================
    /**
     * @brief One node of the tree.
     */
    struct Node
    {
        std::string name;          /**< Displayed name. */
        int type = Unknown;        /**< Item type. */
        int parent = -1;           /**< Parent node, -1 for a subject. */
        int subject = -1;          /**< Subject the node belongs to. */
        int session = -1;          /**< Session the node belongs to, -1 for a subject. */
        std::vector<int> children; /**< Child nodes in display order. */
    };

    //=========================================================================================================
    /**
     * @brief Where add_data_to_session() put an item.
     */
    struct Placement
    {
        int folder = -1;             /**< The data-type folder. */
        bool folder_created = false; /**< Whether the folder was created for the item. */
        int item = -1;               /**< The new item. */
    };

    //=========================================================================================================
    /**
     * @param[in] name  A subject name.
     *
     * @return @p name with the "sub-" prefix and without spaces.
     *
     * @since 3.0.0
     */
    static std::string subject_name(std::string_view name);

    //=========================================================================================================
    /**
     * @param[in] name  A session name.
     *
     * @return @p name with the "ses-" prefix and without spaces.
     *
     * @since 3.0.0
     */
    static std::string session_name(std::string_view name);

    //=========================================================================================================
    /**
     * @param[in] type  Item type of a data item.
     *
     * @return The folder of the data type: "func", "anat", "beh" or "unknown".
     *
     * @since 3.0.0
     */
    static std::string folder_name(int type);

    //=========================================================================================================
    /**
     * Appends a subject.
     *
     * @param[in] name  Subject name; the BIDS prefix is added.
     *
     * @return The new subject.
     *
     * @since 3.0.0
     */
    int add_subject(std::string_view name);

    //=========================================================================================================
    /**
     * Adds a session to every subject called @p subject. With several such subjects they are renamed by
     * appending 1, 2, ... so that they become distinguishable.
     *
     * @param[in] subject   Subject name as displayed.
     * @param[in] session   Session name; the BIDS prefix is added.
     *
     * @return The new sessions, one per matching subject; empty when no subject matches.
     *
     * @since 3.0.0
     */
    std::vector<int> add_session(std::string_view subject, std::string_view session);

    //=========================================================================================================
    /**
     * Adds a session to a subject.
     *
     * @param[in] subject   The subject.
     * @param[in] session   Session name; the BIDS prefix is added.
     *
     * @return The new session.
     *
     * @since 3.0.0
     */
    int add_session(int subject, std::string_view session);

    //=========================================================================================================
    /**
     * Adds a data item to the folder of its type in a session, creating the folder if needed.
     *
     * @param[in] session   The session.
     * @param[in] name      Name of the item.
     * @param[in] type      Item type.
     *
     * @return The folder and the new item.
     *
     * @since 3.0.0
     */
    Placement add_data_to_session(int session, std::string_view name, int type);

    //=========================================================================================================
    /**
     * Adds a sub-item, e.g. an average, to a data item.
     *
     * @param[in] parent    The data item.
     * @param[in] name      Name of the sub-item.
     * @param[in] type      Item type.
     *
     * @return The new sub-item.
     *
     * @since 3.0.0
     */
    int add_to_data(int parent, std::string_view name, int type);

    //=========================================================================================================
    /**
     * Moves a session with everything in it to another subject.
     *
     * @param[in] session   The session.
     * @param[in] subject   The new subject.
     *
     * @since 3.0.0
     */
    void move_session(int session, int subject);

    //=========================================================================================================
    /**
     * Moves a data item into the folder of its type in another session. The old folder is removed when the
     * item was its last one.
     *
     * @param[in] item      The data item.
     * @param[in] session   The new session.
     *
     * @return Where the item went; the item keeps its id.
     *
     * @since 3.0.0
     */
    Placement move_data(int item, int session);

    //=========================================================================================================
    /**
     * Removes a node and everything below it.
     *
     * @param[in] node  The node.
     *
     * @since 3.0.0
     */
    void remove(int node);

    //=========================================================================================================
    /**
     * Renames a node, e.g. after the user edited it.
     *
     * @param[in] node  The node.
     * @param[in] name  New name.
     *
     * @since 3.0.0
     */
    void rename(int node, std::string name);

    //=========================================================================================================
    /**
     * @param[in] node  A node id.
     *
     * @return Whether @p node exists.
     *
     * @since 3.0.0
     */
    bool contains(int node) const;

    //=========================================================================================================
    /**
     * @param[in] node  An existing node.
     *
     * @return The node.
     *
     * @since 3.0.0
     */
    const Node& node(int node) const;

    //=========================================================================================================
    /**
     * @return The subjects in display order.
     *
     * @since 3.0.0
     */
    const std::vector<int>& subjects() const
    {
        return m_subjects;
    }

    //=========================================================================================================
    /**
     * @param[in] node  An existing node.
     *
     * @return Rows from the top level down to @p node.
     *
     * @since 3.0.0
     */
    std::vector<int> path(int node) const;

private:
    int create(std::string name, int type, int parent);
    std::vector<int>& siblings(int node);
    void set_owner(int node, int subject, int session);
    void erase_subtree(int node);

    std::vector<Node> m_nodes;
    std::vector<bool> m_alive;
    std::vector<int> m_subjects;
};

} // namespace mne::viewmodels

#endif // VIEWMODELS_BIDS_TREE_H
