//=============================================================================================================
/**
* @file     fiff.h
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

#ifndef FIFF_H
#define FIFF_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "include/fiff_constants.h"
#include "include/fiff_coord_trans.h"
#include "include/fiff_dir_tree.h"
#include "include/fiff_solution.h"
#include "include/fiff_tag.h"
#include "include/fiff_types.h"



//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>
#include <QList>
#include <QStringList>

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
class FIFFSHARED_EXPORT Fiff
{
public:
    //=========================================================================================================
    /**
    * ctor
    */
    Fiff();

    //=========================================================================================================
    /**
    * dtor
    */
    ~Fiff()
    { }

    //Alphabetic ordered MNE Toolbox fiff_function

    //=========================================================================================================
    /**
    * fiff_dir_tree_find
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree dir_tree_find member function
    */
    static inline QList<FiffDirTree*> dir_tree_find(FiffDirTree* node, fiff_int_t kind)
    {
        return node->dir_tree_find(kind);
    }


    //=========================================================================================================
    /**
    * fiff_invert_transform
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffCoordTrans::invert_transform static function
    */
    static inline bool invert_transform(FiffCoordTrans* p_pTransform)
    {
        return FiffCoordTrans::invert_transform(p_pTransform);
    }

    //=========================================================================================================
    /**
    * fiff_make_dir_tree
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffCoordTrans::invert_transform static function
    */
    static inline qint32 make_dir_tree(QFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0)
    {
        return FiffDirTree::make_dir_tree(p_pFile, p_pDir, p_pTree, start);
    }

    //=========================================================================================================
    /**
    * fiff_open
    *
    * ### MNE toolbox root function ###
    *
    * Opens a fif file and provides the directory of tags
    *
    * @param[in] p_sFileName file name of the file to open
    * @param[out] p_pFile file which is openened
    * @param[out] p_pTree tag directory organized into a tree
    * @param[out] p_pDir the sequential tag directory
    *
    * @return true if succeeded, false otherwise
    */
    static bool open(QString& p_sFileName, QFile*& p_pFile, FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir);

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    */
    static QStringList split_name_list(QString p_sNameList);


    //=========================================================================================================
    /**
    * fiff_read_bad_channels
    *
    * ### MNE toolbox root function ###
    */
    static QStringList read_bad_channels(QFile* p_pFile, FiffDirTree* p_pTree);


    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    */
    static bool read_named_matrix(QFile* p_pFile, FiffDirTree* node, fiff_int_t matkind, FiffSolution*& mat);


    //=========================================================================================================
    /**
    * fiff_read_tag
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffTag::read_tag function
    *
    * Read one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag
    * @param[in] pos position of the tag inside the fif file
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_tag(QFile* p_pFile, FiffTag*& p_pTag, qint64 pos = -1)
    {
        return FiffTag::read_tag(p_pFile, p_pTag, pos);
    }

    //=========================================================================================================
    /**
    * fiff_read_tag_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffTag::read_tag_info function
    *
    * Read tag information of one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag info
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_tag_info(QFile* p_pFile, FiffTag*& p_pTag)
    {
        return FiffTag::read_tag_info(p_pFile, p_pTag);
    }
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FIFF_H
