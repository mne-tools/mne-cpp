//=============================================================================================================
/**
 * @file     fiff_data_ref.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh. All rights reserved.
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
 * @brief    FiffDataRef class declaration.
 *
 */

#ifndef FIFF_DATA_REF_H
#define FIFF_DATA_REF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

#include <QtGlobal>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * External data reference record describing the type, byte order, size, and offset
 * of data stored in an external file.
 *
 * @brief External data reference descriptor.
 */

class FIFFSHARED_EXPORT FiffDataRef
{
public:
    //=========================================================================================================
    /**
     * Default constructor. Initializes all fields to zero.
     */
    FiffDataRef()
        : type(0)
        , endian(0)
        , size(0)
        , offset(0)
    {
    }

    //=========================================================================================================
    /**
     * Destroys the FiffDataRef.
     */
    ~FiffDataRef() = default;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffDataRefRec) 2*int + 2*long = 2*4 + 2*8 = 24.
     *
     * @return the size of the old struct fiffDataRefRec.
     */
    inline static qint32 storageSize();

public:
    qint32  type;       /**< Type of the data. */
    qint32  endian;     /**< Are the data in the little or big endian byte order. */
    qint64  size;       /**< Size of the data, can be over 2 GB. */
    qint64  offset;     /**< Offset to the data in the external file. */
};

//=============================================================================================================
// BACKWARD COMPATIBILITY TYPEDEFS
//=============================================================================================================

/** @brief Backward-compatible typedef for the old fiffDataRefRec struct. */
typedef FiffDataRef fiffDataRefRec;
/** @brief Backward-compatible pointer typedef for the old fiffDataRef pointer. */
typedef FiffDataRef* fiffDataRef;

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffDataRef::storageSize()
{
    return sizeof(FiffDataRef::type) + sizeof(FiffDataRef::endian)
         + sizeof(FiffDataRef::size) + sizeof(FiffDataRef::offset);
}

} // NAMESPACE FIFFLIB

#endif // FIFF_DATA_REF_H
