//=============================================================================================================
/**
* @file     fiff_dir_tree.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    FiffDirTree class declaration, which provides fiff dir tree processing methods.
*
*/

#ifndef FIFF_DIR_TREE_H
#define FIFF_DIR_TREE_H

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_types.h"
#include "fiff_dir_entry.h"
#include "fiff_id.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QSharedPointer>
#include <QStringList>


//*************************************************************************************************************
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
* @brief Directory tree structure
*/
class FIFFSHARED_EXPORT FiffDirTree {

public:
    typedef QSharedPointer<FiffDirTree> SPtr;               /**< Shared pointer type for FiffDirTree. */
    typedef QSharedPointer<const FiffDirTree> ConstSPtr;    /**< Const shared pointer type for FiffDirTree. */

    //=========================================================================================================
    /**
    * Constructors the directory tree structure.
    */
    FiffDirTree();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffDirTree  Directory tree structure which should be copied
    */
    FiffDirTree(const FiffDirTree &p_FiffDirTree);

    //=========================================================================================================
    /**
    * Destroys the fiffDirTree.
    */
    ~FiffDirTree();

    //=========================================================================================================
    /**
    * Initializes directory tree structure.
    */
    void clear();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_copy_tree function
    *
    * Copies directory subtrees from fidin to fidout
    *
    * @param[in] p_pStreamIn    fiff file to copy from
    * @param[in] in_id          file id description
    * @param[out] p_Nodes       subtree directories to be copied
    * @param[in] p_pStreamOut   fiff file to write to
    *
    * @return true if succeeded, false otherwise
    */
    static bool copy_tree(QSharedPointer<FiffStream> p_pStreamIn, FiffId& in_id, QList<FiffDirTree>& p_Nodes, QSharedPointer<FiffStream> p_pStreamOut);

    //=========================================================================================================
    /**
    * Returns true if directory tree structure contains no data.
    *
    * @return true if directory tree structure is empty.
    */
    inline bool isEmpty() const
    {
        return this->nent <= 0;
    }

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_make_dir_tree function
    *
    * Create the directory tree structure
    *
    * @param[in] p_pStream the opened fiff file
    * @param[in] p_Dir the dir entries of which the tree should be constructed
    * @param[out] p_Tree the created dir tree
    * @param[in] start dir entry to start (optional, by default 0)
    *
    * @return index of the last read dir entry
    */
    static qint32 make_dir_tree(FiffStream* p_pStream, QList<FiffDirEntry>& p_Dir, FiffDirTree& p_Tree, qint32 start = 0);

    //=========================================================================================================
    /**
    * ### MNE C function ###: implementation of the fiff_dir_tree_find
    * ### MNE toolbox root function ###: implementation of the fiff_dir_tree_find function
    *
    * Find nodes of the given kind from a directory tree structure
    *
    * @param[in] p_kind the given kind
    *
    * @return list of the found nodes
    */
    QList<FiffDirTree> dir_tree_find(fiff_int_t p_kind) const;

    //=========================================================================================================
    /**
    * ### MNE C function ###: implementation of the fiff_dir_tree_get_tag
    * Implementation of the find_tag function in various files e.g. fiff_read_named_matrix.m,
    *
    * Founds a tag of a given kind within a tree, and reeds it from file.
    * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirTree
    *       class, that's why a tree object doesn't need to be handed to the function.
    *
    * @param[in] p_pStream the opened fif file
    * @param[in] findkind the kind which should be found
    * @param[out] p_pTag the found tag
    *
    * @return true if found, false otherwise
    */
    bool find_tag(FiffStream* p_pStream, fiff_int_t findkind, QSharedPointer<FiffTag>& p_pTag) const;

    //=========================================================================================================
    /**
    * Implementation of the has_tag function in fiff_read_named_matrix.m
    *
    * @param[in] findkind kind to find
    *
    * @return true when fiff_dir_tree contains kind
    */
    bool has_tag(fiff_int_t findkind);

    //=========================================================================================================
    /**
    * Checks whether a DirTree has a specific kind
    *
    * @param[in] findkind kind to find
    *
    * @return true when fiff_dir_tree contains kind
    */
    bool has_kind(fiff_int_t p_kind) const;









//    static void print_id (fiffId id)

//    {
//        printf ("\t%d.%d ",id->version>>16,id->version & 0xFFFF);
//        printf ("0x%x%x ",id->machid[0],id->machid[1]);
//        printf ("%d %d ",id->time.secs,id->time.usecs);
//    }


//    static void print_tree(fiffDirNode node,int indent)

//    {
//        int j,k;
//        int prev_kind,count;
//        fiffDirEntry dentry;

//        if (node == NULL)
//            return;
//        for (k = 0; k < indent; k++)
//            putchar(' ');
//        fiff_explain_block (node->type);
//        printf (" { ");
//        if (node->id != NULL)
//            print_id(node->id);
//        printf ("\n");

//        for (j = 0, prev_kind = -1, count = 0, dentry = node->dir;
//             j < node->nent; j++,dentry++) {
//            if (dentry->kind != prev_kind) {
//                if (count > 1)
//                    printf (" [%d]\n",count);
//                else if (j > 0)
//                    putchar('\n');
//                for (k = 0; k < indent+2; k++)
//                    putchar(' ');
//                fiff_explain (dentry->kind);
//                prev_kind = dentry->kind;
//                count = 1;
//            }
//            else
//                count++;
//            prev_kind = dentry->kind;
//        }
//        if (count > 1)
//            printf (" [%d]\n",count);
//        else if (j > 0)
//            putchar ('\n');
//        for (j = 0; j < node->nchild; j++)
//            print_tree(node->children[j],indent+5);
//        for (k = 0; k < indent; k++)
//            putchar(' ');
//        printf ("}\n");
//    }







public:
    fiff_int_t          block;      /**< Block type for this directory */
    FiffId              id;         /**< Id of this block if any */
    FiffId              parent_id;  /**< Newly added to stay consistent with MATLAB implementation */
    QList<FiffDirEntry> dir;        /**< Directory of tags in this node */
    fiff_int_t          nent;       /**< Number of entries in this node */
    fiff_int_t          nent_tree;  /**< Number of entries in the directory tree node */
    QList<FiffDirTree>  children;   /**< Child nodes */
    fiff_int_t          nchild;     /**< Number of child nodes */

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

} // NAMESPACE

#endif // FIFF_DIR_TREE_H
