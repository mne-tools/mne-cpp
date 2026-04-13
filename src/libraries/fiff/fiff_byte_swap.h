//=============================================================================================================
/**
 * @file     fiff_byte_swap.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Byte-swap utility functions for FIFF binary I/O.
 *
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
