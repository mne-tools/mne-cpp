//=============================================================================================================
/**
* @file     fiff_dir_tree.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the FiffDirTree class declaration, which provides fiff dir tree processing methods.
*
*/

#ifndef FIFF_DIR_TREE_H
#define FIFF_DIR_TREE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"
#include "fiff_constants.h"
#include "fiff_types.h"
#include "fiff_tag.h"
#include "fiff_ctf_comp.h"
#include "fiff_proj.h"
#include "fiff_info.h"
#include "fiff_file.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QList>
#include <QStringList>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffFile;


//=============================================================================================================
/**
* DECLARE CLASS Fiff
*
* @brief The Fiff class provides...
*
* Replaces _fiffDirNode:
* typedef struct _fiffDirNode {
*  int                 type;    /**< Block type for this directory *
*  fiffId              id;      /**< Id of this block if any *
*  fiffDirEntry        dir;     /**< Directory of tags in this node *
*  int                 nent;    /**< Number of entries in this node *
*  fiffDirEntry        dir_tree;    /**< Directory of tags within this node
*                                     * subtrees as well as FIFF_BLOCK_START and FIFF_BLOCK_END
*                   * included. NOTE: While dir is allocated separately
*                   * dir_tree is a pointer to the dirtree field
*                   * in the fiffFile structure. The dir_tree and nent_tree
*                   * fields are only used within the library to facilitate
*                   * certain operations. *
*  int                 nent_tree;   /**< Number of entries in the directory tree node *
*  struct _fiffDirNode *parent;     /**< Parent node *
*  struct _fiffDirNode **children;  /**< Child nodes *
*  int                 nchild;      /**< Number of child nodes *
* } fiffDirNodeRec,*fiffDirNode;    /**< Directory tree structure used by the fiff library routines. *
*/

/**
* FIFF data tag
*
* Tags are used in front of data items to tell what they are.
*/
class FIFFSHARED_EXPORT FiffDirTree {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffDirTree();

    //=========================================================================================================
    /**
    * Destroys the fiffTag.
    */
    ~FiffDirTree();


    //=========================================================================================================
    /**
    *    fiff_copy_tree(fidin, in_id, nodes, fidout)
    *
    *    Copies directory subtrees from fidin to fidout
    */
    static bool copy_tree(FiffFile* fidin, FiffId& in_id, QList<FiffDirTree*>& nodes, FiffFile* fidout)
    {
        if(nodes.size() <= 0)
            return false;

        qint32 k, p;

        for(k = 0; k < nodes.size(); ++k)
        {
            fidout->start_block(nodes[k]->block);//8
            if (nodes[k]->id.version != -1)
            {
                if (in_id.version != -1)
                    fidout->write_id(FIFF_PARENT_FILE_ID, in_id);//9

                fidout->write_id(FIFF_BLOCK_ID);//10
                fidout->write_id(FIFF_PARENT_BLOCK_ID, nodes[k]->id);//11
            }
            for (p = 0; p < nodes[k]->nent; ++p)
            {
                //
                //   Do not copy these tags
                //
                if(nodes[k]->dir[p].kind == FIFF_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_FILE_ID)
                    continue;

                //
                //   Read and write tags, pass data through transparently
                //
                if (!fidin->seek(nodes[k]->dir[p].pos)) //fseek(fidin, nodes(k).dir(p).pos, 'bof') == -1
                {
                    printf("Could not seek to the tag\n");
                    return false;
                }

                FiffTag tag;

                QDataStream in(fidin);
                in.setByteOrder(QDataStream::BigEndian);

                in >> tag.kind;// = fread(fidin, 1, 'int32');
                in >> tag.type;// = fread(fidin, 1, 'uint32');
                in >> tag.size;// = fread(fidin, 1, 'int32');
                in >> tag.next;// = fread(fidin, 1, 'int32');


                if (tag.size > 0)
                {
                    if (tag.data == NULL)
                        tag.data = malloc(tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));
                    else
                        tag.data = realloc(tag.data,tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));

                    if (tag.data == NULL) {
                        printf("fiff_read_tag: memory allocation failed.\n");//consider throw
                        return false;
                    }
                    char *t_pCharData = static_cast< char* >(tag.data);
                    in.readRawData(t_pCharData, tag.size);
                    if (tag.type == FIFFT_STRING)
                        t_pCharData[tag.size] = NULL;//make sure that char ends with NULL
                    FiffTag::fiff_convert_tag_data(&tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
                } //tag.data = fread(fidin, tag.size, 'uchar');


                QDataStream out(fidout);
                out.setByteOrder(QDataStream::BigEndian);

                out << (qint32)tag.kind;
                out << (qint32)tag.type;
                out << (qint32)tag.size;
                out << (qint32)FIFFV_NEXT_SEQ;

                out.writeRawData(static_cast< const char* >(tag.data),tag.size);
            }
            for(p = 0; p < nodes[k]->nchild; ++p)
            {
                QList<FiffDirTree*> childList;
                childList << nodes[k]->children[p];
                FiffDirTree::copy_tree(fidin, in_id, childList, fidout);
            }
            fidout->end_block(nodes[k]->block);
        }
        return true;
    }

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_dir_tree_find function
    */
    static qint32 make_dir_tree(FiffFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: implementation of the fiff_dir_tree_find function
    *
    * Find nodes of the given kind from a directory tree structure
    *
    * @param[in] kind the given kind
    *
    * @return list of the found nodes
    */
    QList<FiffDirTree*> dir_tree_find(fiff_int_t kind);

    //=========================================================================================================
    /**
    * Implementation of the find_tag function in various files e.g. fiff_read_named_matrix.m
    *
    * Founds a tag of a given kind within a tree, and reeds it from file.
    * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirTree
    *       class, that's why a tree object doesn't need to be handed to the function.
    *
    * @param[in] p_pFile the opened fif file
    * @param[in] findkind the kind which should be found
    * @param[out] p_pTag the found tag
    *
    * @return true if found, false otherwise
    */
    bool find_tag(FiffFile* p_pFile, fiff_int_t findkind, FiffTag*& p_pTag);

    //=========================================================================================================
    /**
    * Implementation of the has_tag function in fiff_read_named_matrix.m
    */
    bool has_tag(fiff_int_t findkind);

    //=========================================================================================================
    /**
    * fiff_read_bad_channels
    *
    * ### MNE toolbox root function ###
    *
    * Reads the bad channel list from a node if it exists
    * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirTree
    *       class, that's why a tree object doesn't need to be handed to the function.
    *
    *
    * @param[in] p_pFile The opened fif file to read from
    *
    * @return the bad channel list
    */
    QStringList read_bad_channels(FiffFile* p_pFile);

    //=========================================================================================================
    /**
    * fiff_read_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * [ compdata ] = fiff_read_ctf_comp(fid,node,chs)
    *
    * Read the CTF software compensation data from the given node
    *
    */
    QList<FiffCtfComp*> read_ctf_comp(FiffFile* p_pFile, QList<FiffChInfo>& chs);

    //=========================================================================================================
    /**
    * fiff_read_meas_info
    *
    * ### MNE toolbox root function ###
    *
    * [info,meas] = fiff_read_meas_info(source,tree)
    *
    * Read the measurement info
    *
    * If tree is specified, source is assumed to be an open file id,
    * otherwise a the name of the file to read. If tree is missing, the
    * meas output argument should not be specified.
    *
    */
    FiffDirTree* read_meas_info(FiffFile* p_pFile, FiffInfo*& info);

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    */
    bool read_named_matrix(FiffFile* p_pFile, fiff_int_t matkind, FiffNamedMatrix*& mat);

    //=========================================================================================================
    /**
    * fiff_read_proj
    *
    * ### MNE toolbox root function ###
    *
    * [ projdata ] = fiff_read_proj(fid,node)
    *
    * Read the SSP data under a given directory node
    *
    */
    QList<FiffProj*> read_proj(FiffFile* p_pFile);

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    */
    static QStringList split_name_list(QString p_sNameList);

public:
    fiff_int_t              block;      /**< Block type for this directory */
    FiffId                  id;         /**< Id of this block if any */
    FiffId                  parent_id;  /**< Newly added to stay consistent with MATLAB implementation */
    QList<fiff_dir_entry_t> dir;        /**< Directory of tags in this node */
    fiff_int_t              nent;       /**< Number of entries in this node */
    fiff_int_t              nent_tree;  /**< Number of entries in the directory tree node */
    QList<FiffDirTree*>     children;   /**< Child nodes */
    fiff_int_t              nchild;     /**< Number of child nodes */

};

} // NAMESPACE

#endif // FIFF_DIR_TREE_H
