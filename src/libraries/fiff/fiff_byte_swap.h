//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_byte_swap.h
 * @since March 2026
 * @brief Endianness swap helpers for the FIFF binary tag I/O layer (FIFF is always written big-endian on disk).
 *
 * FIFF tags are stored in network (big-endian) byte order on disk while
 * all currently supported host platforms are little-endian. @ref FiffStream
 * therefore funnels every scalar / array read or write through one of
 * these tiny inline helpers, which gives both the per-value variants
 * (@c swap_short, @c swap_int, @c swap_long, @c swap_float, @c swap_double)
 * and the in-place pointer variants used when decoding densely packed
 * matrix payloads where allocating a copy would be wasteful.
 *
 * These functions intentionally mirror the @c swap_*() helpers in the MNE-C
 * @c fiff_io.c and the @c numpy ``>i4`` / ``>f4`` dtype handling in
 * MNE-Python so a tag written by any of the three implementations
 * round-trips byte-for-byte through the others.
 */

#ifndef FIFF_BYTE_SWAP_H
#define FIFF_BYTE_SWAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtGlobal>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Swap a 16-bit short.
 */
inline qint16 swap_short(qint16 source)
{
    auto *csource = reinterpret_cast<unsigned char *>(&source);
    qint16 result;
    auto *cresult = reinterpret_cast<unsigned char *>(&result);
    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return result;
}

//=============================================================================================================
/**
 * Swap a 32-bit integer.
 */
inline qint32 swap_int(qint32 source)
{
    auto *csource = reinterpret_cast<unsigned char *>(&source);
    qint32 result;
    auto *cresult = reinterpret_cast<unsigned char *>(&result);
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return result;
}

//=============================================================================================================
/**
 * Swap a 32-bit integer in place.
 */
inline void swap_intp(qint32 *source)
{
    auto *csource = reinterpret_cast<unsigned char *>(source);
    unsigned char c;
    c = csource[3]; csource[3] = csource[0]; csource[0] = c;
    c = csource[2]; csource[2] = csource[1]; csource[1] = c;
}

//=============================================================================================================
/**
 * Swap a 64-bit long.
 */
inline qint64 swap_long(qint64 source)
{
    auto *csource = reinterpret_cast<unsigned char *>(&source);
    qint64 result;
    auto *cresult = reinterpret_cast<unsigned char *>(&result);
    cresult[0] = csource[7];
    cresult[1] = csource[6];
    cresult[2] = csource[5];
    cresult[3] = csource[4];
    cresult[4] = csource[3];
    cresult[5] = csource[2];
    cresult[6] = csource[1];
    cresult[7] = csource[0];
    return result;
}

//=============================================================================================================
/**
 * Swap a 64-bit long in place.
 */
inline void swap_longp(qint64 *source)
{
    auto *csource = reinterpret_cast<unsigned char *>(source);
    unsigned char c;
    c = csource[0]; csource[0] = csource[7]; csource[7] = c;
    c = csource[1]; csource[1] = csource[6]; csource[6] = c;
    c = csource[2]; csource[2] = csource[5]; csource[5] = c;
    c = csource[3]; csource[3] = csource[4]; csource[4] = c;
}

//=============================================================================================================
/**
 * Swap a 32-bit float.
 */
inline float swap_float(float source)
{
    auto *csource = reinterpret_cast<unsigned char *>(&source);
    float result;
    auto *cresult = reinterpret_cast<unsigned char *>(&result);
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return result;
}

//=============================================================================================================
/**
 * Swap a 32-bit float in place.
 */
inline void swap_floatp(float *source)
{
    auto *csource = reinterpret_cast<unsigned char *>(source);
    unsigned char c;
    c = csource[3]; csource[3] = csource[0]; csource[0] = c;
    c = csource[2]; csource[2] = csource[1]; csource[1] = c;
}

//=============================================================================================================
/**
 * Swap a 64-bit double in place.
 */
inline void swap_doublep(double *source)
{
    auto *csource = reinterpret_cast<unsigned char *>(source);
    unsigned char c;
    c = csource[7]; csource[7] = csource[0]; csource[0] = c;
    c = csource[6]; csource[6] = csource[1]; csource[1] = c;
    c = csource[5]; csource[5] = csource[2]; csource[2] = c;
    c = csource[4]; csource[4] = csource[3]; csource[3] = c;
}

} // NAMESPACE FIFFLIB

#endif // FIFF_BYTE_SWAP_H
