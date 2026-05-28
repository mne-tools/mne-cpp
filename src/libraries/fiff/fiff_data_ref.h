//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_data_ref.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    External-data reference record describing type, endianness, size and offset of an out-of-file FIFF payload.
 *
 * A @c FIFF_REF_ROLE / @c FIFF_DATA_REF tag carries a small fixed-size
 * record (the original @c fiffDataRefRec, 24 bytes: two 32-bit ints
 * followed by two 64-bit offsets) that points at a payload living in a
 * separate file rather than inline in the FIFF tag stream. This is how
 * the Neuromag acquisition system splits very long recordings across
 * multiple physical files while keeping a single logical FIFF tree, and
 * how MNE-Python's @c mne.io.Raw chains @c .fif fragments through the
 * @c next_fname mechanism. @ref FiffDataRef is the in-memory representation
 * of that record; @ref FiffDataRef::storageSize matches the on-disk size
 * exactly so @ref FiffStream can stream the data ref directly into a
 * buffer with no per-field marshalling.
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
 * @brief External data reference: type, endian, byte size and offset into an external FIFF payload file.
 *
 * Mirrors the original @c fiffDataRefRec exactly: a 24-byte record made of
 * two @c qint32 (type, endianness flag) followed by two @c qint64 (payload
 * size in bytes, byte offset into the external file). The 64-bit fields
 * let referenced payloads exceed 2 GiB, which is required by long
 * continuous Neuromag recordings split into multi-file FIFF trees.
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
