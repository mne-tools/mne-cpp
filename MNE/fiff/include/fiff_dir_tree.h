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
* @brief    ToDo Documentation...
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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{


//=============================================================================================================
/**
* DECLARE CLASS Fiff
*
* @brief The Fiff class provides...
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
    * ### MNE toolbox root function ###: Implementation of the fiff_dir_tree_find function
    */
    static qint32 make_dir_tree(QFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: implementation of the fiff_dir_tree_find function
    */
    QList<FiffDirTree*> dir_tree_find(fiff_int_t kind);

    //=========================================================================================================
    /**
    * Implementation of the find_tag function in various files e.g. fiff_read_named_matrix.m
    */
    bool find_tag(QFile* p_pFile, fiff_int_t findkind, FiffTag*& p_pTag);

    //=========================================================================================================
    /**
    * Implementation of the has_tag function in fiff_read_named_matrix.m
    */
    bool has_tag(fiff_int_t findkind);

public:
    qint32                  block;      /**< Block type for this directory */
    fiff_id_t               id;         /**< Id of this block if any */
    fiff_id_t               parent_id;  /**< Newly added to stay consistent with MATLAB implementation */
    QList<fiff_dir_entry_t> dir;        /**< Directory of tags in this node */
    qint32                  nent;       /**< Number of entries in this node */
    qint32                  nent_tree;  /**< Number of entries in the directory tree node */
    QList<FiffDirTree*>     children;   /**< Child nodes */
    qint32                  nchild;     /**< Number of child nodes */

};

} // NAMESPACE

#endif // FIFF_DIR_TREE_H
