//=============================================================================================================
/**
 * @file     fiff_dir_node.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    FiffDirNode class declaration, which provides fiff dir tree processing methods.
 *
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
#include "fiff_explain.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QSharedPointer>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffStream;
class FiffTag;

//=============================================================================================================
/**
 * Replaces _fiffDirNode struct fiffDirNodeRec,*fiffDirNode
 *
 * @brief Directory Node structure
 */
class FIFFSHARED_EXPORT FiffDirNode {
public:
    typedef QSharedPointer<FiffDirNode> SPtr;               /**< Shared pointer type for FiffDirNode. */
    typedef QSharedPointer<const FiffDirNode> ConstSPtr;    /**< Const shared pointer type for FiffDirNode. */

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
     * ### MNE toolbox root function ###: Definition of the fiff_copy_tree function
     *
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
     * ### MNE C function ###: Definition of the fiff_dir_tree_find
     * ### MNE toolbox root function ###: Definition of the fiff_dir_tree_find function
     *
     * Find nodes of the given kind from a directory tree structure
     *
     * @param[in] p_kind the given kind.
     *
     * @return list of the found nodes.
     */
    QList<FiffDirNode::SPtr> dir_tree_find(fiff_int_t p_kind) const;

    //=========================================================================================================
    /**
     * ### MNE C function ###: Definition of the fiff_dir_tree_get_tag
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
    inline bool find_tag(QSharedPointer<FiffStream>& p_pStream, fiff_int_t findkind, QSharedPointer<FiffTag>& p_pTag) const;

    //=========================================================================================================
    /**
     * ### MNE C function ###: Definition of the fiff_dir_tree_get_tag
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
    bool find_tag(FiffStream* p_pStream, fiff_int_t findkind, QSharedPointer<FiffTag>& p_pTag) const;

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
     * Refactored: print_tree (fiff_dir_tree.c)
     *
     * @param[in] indent     number of intendations.
     */
    void print(int indent) const;

    //=========================================================================================================
    /**
     * Try to explain a block...
     * Refactored: fiff_explain_block (fiff_explain.c)
     *
     * @param[in] kind   Block kind.
     */
    static void explain_block(int kind);

    //=========================================================================================================
    /**
     * Try to explain...
     * Refactored: fiff_explain (fiff_explain.c)
     *
     * @param[in] kind   directory kind.
     */
    static void explain (int kind);

    //=========================================================================================================
    /**
     * Get textual explanation of a tag
     * Refactored: fiff_get_tag_explanation (fiff_explain.c)
     *
     * @param[in] kind   directory kind.
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

    // typedef struct _fiffDirNode {
    //  int                 type;    /**< Block type for this directory *
    //  fiffId              id;      /**< Id of this block if any *
    //  fiffDirEntry        dir;     /**< Directory of tags in this node *
    //  int                 nent;    /**< Number of entries in this node *
    //  fiffDirEntry        dir_tree;    /**< Directory of tags within this node
    //                                     * subtrees as well as FIFF_BLOCK_START and FIFF_BLOCK_END
    //                   * included. NOTE: While dir is allocated separately
    //                   * dir_tree is a pointer to the dirtree field
    //                   * in the FiffStream structure. The dir_tree and nent_tree
    //                   * fields are only used within the library to facilitate
    //                   * certain operations. *
    //  int                 nent_tree;   /**< Number of entries in the directory tree node *
    //  struct _fiffDirNode *parent;     /**< Parent node *
    //  struct _fiffDirNode **children;  /**< Child nodes *
    //  int                 nchild;      /**< Number of child nodes *
    // } fiffDirNodeRec,*fiffDirNode;    /**< Directory tree structure used by the fiff library routines. *
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffDirNode::find_tag(QSharedPointer<FiffStream> &p_pStream, fiff_int_t findkind, QSharedPointer<FiffTag> &p_pTag) const
{
    return find_tag(p_pStream.data(), findkind, p_pTag);
}
} // NAMESPACE

#endif // FIFF_DIR_NODE_H
