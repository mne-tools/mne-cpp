//=============================================================================================================
/**
 * @file     fiff_raw_dir.h
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
 * @brief    FiffRawDir class declaration.
 *
 */

#ifndef FIFF_RAW_DIR_H
#define FIFF_RAW_DIR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_dir_entry.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Special fiff diretory entry for raw data. ToDo: derive this of FiffDirEntry.
 *
 *
 * @brief Raw Directory entry
 */
class FIFFSHARED_EXPORT FiffRawDir {

public:
    typedef QSharedPointer<FiffRawDir> SPtr;            /**< Shared pointer type for FiffRawDir. */
    typedef QSharedPointer<const FiffRawDir> ConstSPtr; /**< Const shared pointer type for FiffRawDir. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    FiffRawDir();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffRawDir   Raw directory entry which should be copied.
     */
    FiffRawDir(const FiffRawDir &p_FiffRawDir);

    //=========================================================================================================
    /**
     * Destroys the raw Directory entry.
     */
    ~FiffRawDir();

public:
    FiffDirEntry::SPtr  ent;    /**< Directory entry description. */
    fiff_int_t          first;  /**< first sample. */
    fiff_int_t          last;   /**< last sample. */
    fiff_int_t          nsamp;  /**< Number of samples. */
};
} // NAMESPACE

#endif // FIFF_RAW_DIR_H
