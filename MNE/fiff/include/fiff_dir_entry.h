//=============================================================================================================
/**
* @file     fiff_dir_entry.h
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
* @brief    Contains the FiffDirEntry class declaration.
*
*/

#ifndef FIFF_DIR_ENTRY_H
#define FIFF_DIR_ENTRY_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS FiffDirEntry, replaces fiffDirEntryRec which had a size of 4*4 = 16
*
* A Dir Entry.
*
* /** Directories are composed of these structures. *
* typedef struct _fiffDirEntryRec {
*  fiff_int_t  kind;		/**< Tag number *
*  fiff_int_t  type;		/**< Data type *
*  fiff_int_t  size;		/**< How many bytes *
*  fiff_int_t  pos;		/**< Location in file
*                  * Note: the data is located at pos +
*                  * FIFFC_DATA_OFFSET *
* } fiffDirEntryRec,*fiffDirEntry;/**< Directory is composed of these *
* /** Alias for fiffDirEntryRec *
* typedef fiffDirEntryRec fiff_dir_entry_t;
*
* @brief The FiffId class provides the fiff file id description
**/

class FIFFSHARED_EXPORT FiffDirEntry {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffDirEntry()
    : kind(-1)
    , type(-1)
    , size(-1)
    , pos(-1)
    {

    }

    //=========================================================================================================
    /**
    * Destroys the FiffInfo.
    */
    ~FiffDirEntry()
    {

    }

    //=========================================================================================================
    /**
    * Size of the old struct (fiffDirEntryRec) 4*int = 4*4 = 16
    *
    * @return the size of the old struct fiffDirEntryRec.
    */
    inline static qint32 storageSize();

public:
    fiff_int_t  kind;   /**< Tag number */
    fiff_int_t  type;   /**< Data type */
    fiff_int_t  size;   /**< How many bytes */
    fiff_int_t  pos;    /**< Location in file; Note: the data is located at pos + FIFFC_DATA_OFFSET */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffDirEntry::storageSize()
{
    return 16;
}

} // NAMESPACE

#endif // FIFF_DIR_ENTRY_H
