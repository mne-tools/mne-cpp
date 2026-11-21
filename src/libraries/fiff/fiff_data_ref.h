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
 * @c next_fname mechanism. @ref mne::FiffDataRef is the in-memory representation
 * of that record; @ref mne::FiffDataRef::storage_size "FiffDataRef::storage_size" matches the on-disk size
 * exactly so @ref mne::FiffStream can stream the data ref directly into a
 * buffer with no per-field marshalling.
 */

#ifndef FIFF_DATA_REF_H
#define FIFF_DATA_REF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief External data reference: type, endian, byte size and offset into an external FIFF payload file.
 *
 * Mirrors the original @c fiffDataRefRec exactly: a 24-byte record made of
 * two @c std::int32_t (type, endianness flag) followed by two @c std::int64_t (payload
 * size in bytes, byte offset into the external file). The 64-bit fields
 * let referenced payloads exceed 2 GiB, which is required by long
 * continuous Neuromag recordings split into multi-file FIFF trees.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_data_ref_usage
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
    inline static std::int32_t storage_size();

public:
    std::int32_t type;   /**< Type of the data. */
    std::int32_t endian; /**< Are the data in the little or big endian byte order. */
    std::int64_t size;   /**< Size of the data, can be over 2 GB. */
    std::int64_t offset; /**< Offset to the data in the external file. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FiffDataRef::storage_size()
{
    return sizeof(FiffDataRef::type) + sizeof(FiffDataRef::endian) + sizeof(FiffDataRef::size) + sizeof(FiffDataRef::offset);
}

} // NAMESPACE mne_fiff

#endif // FIFF_DATA_REF_H
