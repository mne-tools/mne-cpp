//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fiff_dir_node.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Recursive node of the parsed FIFF block tree (FIFFB_* hierarchy with directory entries and children).
 *
 * A FIFF file is a flat tag stream bracketed by @c FIFFB_BLOCK_START /
 * @c FIFFB_BLOCK_END markers. @ref FiffDirNode is the tree the stream
 * parser produces from those brackets: each node owns its block kind,
 * its @ref FiffId, the list of @ref FiffDirEntry records that live
 * directly inside its block, and a list of child @ref FiffDirNode
 * sub-blocks. Together with the random-access directory at the tail of
 * the file this lets every downstream reader (@ref FiffRawData,
 * @ref FiffEvoked, @ref FiffInfo, @ref FiffCov, ...) navigate to its
 * block of interest with one call to @ref FiffDirNode::dir_tree_find
 * rather than rescanning the tag stream.
 *
 * Mirrors the @c dir_tree object returned by
 * @c mne.io.tree.dir_tree_find in MNE-Python so the same FIFFB_* search
 * patterns can be used in both implementations.
 */

#ifndef FIFF_DIR_NODE_H
#define FIFF_DIR_NODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_types.h"
#include "fiff_dir_entry.h"
#include "fiff_id.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffStream;
class FiffTag;

//=============================================================================================================
/**
 * @brief Recursive FIFF block-tree node: block kind, block ID, directly contained directory entries and child sub-blocks.
 *
 * Each node corresponds to one @c FIFFB_BLOCK_START ... @c FIFFB_BLOCK_END
 * pair in the on-disk tag stream. The full tree is built once by
 * @ref FiffStream::make_dir_tree after the directory tag is parsed, and is
 * then traversed by every higher-level reader (@ref FiffInfo,
 * @ref FiffRawData, @ref FiffEvoked, @ref FiffCov, ...) via
 * @ref dir_tree_find. The static @ref copy_tree helper streams an entire
 * sub-tree from one @ref FiffStream to another preserving block IDs,
 * which is how MNE-CPP writes derived files that keep provenance back to
 * the source recording.
 */
class FIFFSHARED_EXPORT FiffDirNode {
public:
    using SPtr = QSharedPointer<FiffDirNode>;            /**< Shared pointer type for FiffDirNode. */
    using ConstSPtr = QSharedPointer<const FiffDirNode>; /**< Const shared pointer type for FiffDirNode. */
    using UPtr = std::unique_ptr<FiffDirNode>;             /**< Unique pointer type for FiffDirNode. */
    using ConstUPtr = std::unique_ptr<const FiffDirNode>;  /**< Const unique pointer type for FiffDirNode. */

    //=========================================================================================================
    /**
     * Constructors the directory tree structure.
     */
    FiffDirNode();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDirTree  Directory tree structure which should be copied.
     */
    FiffDirNode(const FiffDirNode* p_FiffDirTree);

    //=========================================================================================================
    /**
     * Destroys the fiffDirTree.
     */
    ~FiffDirNode();

    //=========================================================================================================
    /**
     * Copies directory subtrees from fidin to fidout
     *
     * @param[in] p_pStreamIn    fiff file to copy from.
     * @param[in] in_id          file id description.
     * @param[in] p_Nodes        subtree directories to be copied.
     * @param[out] p_pStreamOut   fiff file to write to.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool copy_tree(QSharedPointer<FiffStream>& p_pStreamIn, const FiffId& in_id, const QList< QSharedPointer<FiffDirNode> >& p_Nodes, QSharedPointer<FiffStream>& p_pStreamOut);

    //=========================================================================================================
    /**
     * Returns true if directory tree structure contains no data.
     *
     * @return true if directory tree structure is empty.
     */
    inline bool isEmpty() const
    {
        return this->type < 0;
    }

    //=========================================================================================================
    /**
     * Find nodes of the given kind from a directory tree structure
     *
     * @param[in] p_kind the given kind.
     *
     * @return list of the found nodes.
     */
    QList<FiffDirNode::SPtr> dir_tree_find(fiff_int_t p_kind) const;

    //=========================================================================================================
    /**
     * Definition of the find_tag function in various files e.g. fiff_read_named_matrix.m,
     *
     * Founds a tag of a given kind within a tree, and reeds it from file.
     * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirNode
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * @param[in] p_pStream the opened fif file.
     * @param[in] findkind the kind which should be found.
     * @param[out] p_pTag the found tag.
     *
     * @return true if found, false otherwise.
     */
    inline bool find_tag(QSharedPointer<FiffStream>& p_pStream, fiff_int_t findkind, std::unique_ptr<FiffTag>& p_pTag) const;

    //=========================================================================================================
    /**
     * Definition of the find_tag function in various files e.g. fiff_read_named_matrix.m,
     *
     * Founds a tag of a given kind within a tree, and reeds it from file.
     * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirNode
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * @param[in] p_pStream the opened fif file.
     * @param[in] findkind the kind which should be found.
     * @param[out] p_pTag the found tag.
     *
     * @return true if found, false otherwise.
     */
    bool find_tag(FiffStream* p_pStream, fiff_int_t findkind, std::unique_ptr<FiffTag>& p_pTag) const;

    //=========================================================================================================
    /**
     * Definition of the has_tag function in fiff_read_named_matrix.m
     *
     * @param[in] findkind kind to find.
     *
     * @return true when fiff_dir_node contains kind.
     */
    bool has_tag(fiff_int_t findkind);

    //=========================================================================================================
    /**
     * Checks whether a FiffDirNode has a specific kind
     *
     * @param[in] findkind kind to find.
     *
     * @return true when fiff_dir_node contains kind.
     */
    bool has_kind(fiff_int_t p_kind) const;

    //=========================================================================================================
    /**
     * Prints elements of a tree.
     *
     * @param[in] indent     number of intendations.
     */
    void print(int indent) const;

    //=========================================================================================================
    /**
     * Try to explain a block...
     *
     * @param[in] kind   Block kind.
     */
    static void explain_block(int kind);

    //=========================================================================================================
    /**
     * Try to explain...
     *
     * @param[in] kind   directory kind.
     */
    static void explain (int kind);

    //=========================================================================================================
    /**
     * Get textual explanation of a tag
     *
     * @param[in] kind   directory kind.
     *
     * @return Pointer to the static string explaining the tag, or nullptr if not found.
     */
    static const char *get_tag_explanation (int kind);

    //=========================================================================================================
    /**
     * Returns the number of entries in this node
     *
     * @return Number of entries in this node.
     */
    fiff_int_t nent() const;

    //=========================================================================================================
    /**
     * Returns the number of child nodes
     *
     * @return Number of child nodes.
     */
    fiff_int_t nchild() const;

public:
    fiff_int_t                  type;       /**< Block type for this directory. */
    FiffId                      id;         /**< Id of this block if any. */
    QList<FiffDirEntry::SPtr>   dir;        /**< Directory of tags in this node. */
//    fiff_int_t                  nent;       /**< Number of entries in this node. */
    QList<FiffDirEntry::SPtr>   dir_tree;   /**< Directory of tags within this node subtrees
                                                 as well as FIFF_BLOCK_START and FIFF_BLOCK_END */
    fiff_int_t                  nent_tree;  /**< Number of entries in the directory tree node. */
    FiffDirNode::SPtr           parent;     /**< Parent node. */
    FiffId                      parent_id;  /**< Newly added to stay consistent with MATLAB implementation. */
    QList<FiffDirNode::SPtr>    children;   /**< Child nodes. */
//    fiff_int_t                  nchild;     /**< Number of child nodes. */ -> use nchild() instead

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffDirNode::find_tag(QSharedPointer<FiffStream> &p_pStream, fiff_int_t findkind, std::unique_ptr<FiffTag> &p_pTag) const
{
    return find_tag(p_pStream.data(), findkind, p_pTag);
}
} // NAMESPACE

#endif // FIFF_DIR_NODE_H
