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
 * @c FIFFB_BLOCK_END markers. @ref mne::FiffDirNode is the tree the stream
 * parser produces from those brackets: each node owns its block kind,
 * its @ref mne::FiffId, the list of @ref mne::FiffDirEntry records that live
 * directly inside its block, and a list of child @ref mne::FiffDirNode
 * sub-blocks. Together with the random-access directory at the tail of
 * the file this lets every downstream reader (@ref mne::FiffRawData,
 * @ref mne::FiffEvoked, @ref mne::FiffInfo, @ref mne::FiffCov, ...) navigate to its
 * block of interest with one call to @ref mne::FiffDirNode::dir_tree_find "FiffDirNode::dir_tree_find"
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
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <ostream>
#include <optional>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffStream;
class FiffTag;

//=============================================================================================================
/**
 * @brief Recursive FIFF block-tree node: block kind, block ID, directly contained directory entries and child sub-blocks.
 *
 * Each node corresponds to one @c FIFFB_BLOCK_START ... @c FIFFB_BLOCK_END
 * pair in the on-disk tag stream. The full tree is built once by
 * @c FiffStream::make_dir_tree after the directory tag is parsed, and is
 * then traversed by every higher-level reader (@ref FiffInfo,
 * @ref FiffRawData, @ref FiffEvoked, @ref FiffCov, ...) via
 * @ref dir_tree_find. The static @ref copy_tree helper streams an entire
 * sub-tree from one @ref FiffStream to another preserving block IDs,
 * which is how MNE-CPP writes derived files that keep provenance back to
 * the source recording.
 *
 * @snippet ex_fiff_api/main.cpp fiff_stream_usage
 *
 * @snippet ex_fiff_api/main.cpp fiff_explain_usage
 */
class FIFFSHARED_EXPORT FiffDirNode : public std::enable_shared_from_this<FiffDirNode>
{
public:
    //=========================================================================================================
    /**
     * Constructors the directory tree structure.
     */
    FiffDirNode();

    //=========================================================================================================
    /**
     * Destroys the fiffDirTree.
     */
    ~FiffDirNode();

    //=========================================================================================================
    /**
     * Copies directory subtrees from fidin to fidout
     *
     * @param[in] source         fiff file to copy from.
     * @param[in] in_id          file id description.
     * @param[in] p_Nodes        subtree directories to be copied.
     * @param[in] target         fiff file to write to.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool copy_tree(FiffStream& source, const FiffId& in_id, std::span<const std::shared_ptr<FiffDirNode>> p_Nodes, FiffStream& target);

    //=========================================================================================================
    /**
     * Returns true if directory tree structure contains no data.
     *
     * @return true if directory tree structure is empty.
     */
    bool empty() const
    {
        return this->type < 0;
    }

    //=========================================================================================================
    /**
     * Finds the blocks of a kind in this block and its sub-blocks, in file order.
     *
     * @param[in] p_kind the given kind.
     *
     * @return The blocks of the tree itself, not copies.
     */
    std::vector<std::shared_ptr<FiffDirNode>> dir_tree_find(fiff_int_t p_kind);


    //=========================================================================================================
    /**
     * Reads the first tag of a kind among the tags of this block.
     *
     * @param[in] stream     The file the block belongs to.
     * @param[in] findkind   The tag kind.
     *
     * @return The tag, or nothing if the block holds none of that kind.
     */
    [[nodiscard]] std::optional<FiffTag> find_tag(FiffStream& stream, fiff_int_t findkind) const;

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
     * @param[in] p_kind kind to find.
     *
     * @return true when fiff_dir_node contains kind.
     */
    bool has_kind(fiff_int_t p_kind) const;

    //=========================================================================================================
    /**
     * Prints this block and its sub-blocks like MNE-C's fiff_dir_tree: one line per block with its id, one
     * line per run of equal tag kinds with the run length in brackets, sub-blocks indented by five.
     *
     * @param[in] out        Stream to print to.
     * @param[in] indent     Indentation of this block.
     */
    void print(std::ostream& out, int indent = 0) const;

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
    fiff_int_t type;                                    /**< Block type for this directory. */
    FiffId id;                                          /**< Id of this block if any. */
    std::vector<FiffDirEntry> dir;                      /**< Directory of tags in this node. */
    std::weak_ptr<FiffDirNode> parent;                  /**< Parent node; weak, since the parent owns its children. */
    FiffId parent_id;                                   /**< Newly added to stay consistent with MATLAB implementation. */
    std::vector<std::shared_ptr<FiffDirNode>> children; /**< Child nodes. */

private:
    void collect(fiff_int_t kind, std::vector<std::shared_ptr<FiffDirNode>>& found);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FIFF_DIR_NODE_H
