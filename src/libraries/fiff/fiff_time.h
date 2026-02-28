//=============================================================================================================
/**
 * @file     fiff_time.h
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
 * @brief    FiffTime class declaration.
 *
 */

#ifndef FIFF_TIME_H
#define FIFF_TIME_H

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
 * Accurate time stamps used in FIFF files.
 *
 * @brief Time stamp record storing seconds and microseconds since epoch.
 */

class FIFFSHARED_EXPORT FiffTime
{
public:
    //=========================================================================================================
    /**
     * Default constructor. Initializes both fields to zero.
     */
    FiffTime()
        : secs(0)
        , usecs(0)
    {
    }

    //=========================================================================================================
    /**
     * Parameterized constructor.
     *
     * @param[in] p_secs    GMT time in seconds since epoch.
     * @param[in] p_usecs   Fraction of seconds in microseconds.
     */
    FiffTime(qint32 p_secs, qint32 p_usecs)
        : secs(p_secs)
        , usecs(p_usecs)
    {
    }

    //=========================================================================================================
    /**
     * Destroys the FiffTime.
     */
    ~FiffTime() = default;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffTimeRec) 2*int = 2*4 = 8.
     *
     * @return the size of the old struct fiffTimeRec.
     */
    inline static qint32 storageSize();

public:
    qint32 secs;    /**< GMT time in seconds since epoch. */
    qint32 usecs;   /**< Fraction of seconds in microseconds. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffTime::storageSize()
{
    return sizeof(FiffTime::secs) + sizeof(FiffTime::usecs);
}

//=============================================================================================================

/**
 * Compares two FiffTime instances for equality.
 *
 * @param[in] a   First time value.
 * @param[in] b   Second time value.
 *
 * @return true if both secs and usecs are equal.
 */
inline bool operator==(const FiffTime& a, const FiffTime& b)
{
    return (a.secs == b.secs && a.usecs == b.usecs);
}

//=============================================================================================================

/**
 * Compares two FiffTime instances for inequality.
 *
 * @param[in] a   First time value.
 * @param[in] b   Second time value.
 *
 * @return true if secs or usecs differ.
 */
inline bool operator!=(const FiffTime& a, const FiffTime& b)
{
    return !(a == b);
}

} // NAMESPACE FIFFLIB

#endif // FIFF_TIME_H
