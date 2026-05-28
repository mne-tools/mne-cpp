//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_time.h
 * @since February 2026
 * @brief Compact seconds + microseconds time record used by FIFF measurement, event and HPI tags.
 *
 * Replaces the legacy @c fiffTimeRec struct from MNE-C with a small C++
 * class whose layout matches the on-disk pair of 32-bit integers stored in
 * @c FIFF_MEAS_DATE, @c FIFF_DATA_BUFFER timestamps and continuous-HPI
 * position records. Times are represented as Unix seconds since the
 * 1970-01-01T00:00:00 UTC epoch plus a microsecond fraction, the same
 * convention used by @c mne.io.meas_info in MNE-Python so date round-trip
 * between the two libraries is loss-less to microsecond resolution.
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
 * @brief FIFF time stamp: Unix seconds plus a microsecond fraction matching the on-disk @c fiffTimeRec record.
 *
 * Layout is two 32-bit fields (secs, usecs) — the exact representation
 * used by @c FIFF_MEAS_DATE and the per-buffer timestamps embedded in raw
 * data blocks, so an instance can be streamed in and out without
 * per-field marshalling.
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
