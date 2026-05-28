//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_ch_pos.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Coil-frame position record embedded inside FIFF_CH_INFO: coil location and 3x3 EX/EY/EZ orientation triad.
 *
 * A FIFF channel info record carries a sub-record describing where the
 * sensing element sits in the device coordinate frame (@c FIFFV_COORD_DEVICE)
 * and how it is oriented. @ref FiffChPos owns that sub-record: a 3-vector
 * @c r0 with the coil origin in metres plus three orthonormal 3-vectors
 * @c ex, @c ey, @c ez encoding the coil-local axes (typically used by the
 * forward solution to integrate over the coil pickup area).
 *
 * Field layout matches the @c fiffChPosRec struct exactly so the record
 * can be streamed without per-field marshalling, and it is the C++
 * counterpart of the @c loc array stored in @c mne.io.Info channel
 * entries in MNE-Python.
 */

#ifndef FIFF_CH_POS_H
#define FIFF_CH_POS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Channel coil-frame placement: origin @c r0 (m) and orthonormal axes @c ex / @c ey / @c ez in @c FIFFV_COORD_DEVICE.
 *
 * Twelve floats laid out exactly like the @c fiffChPosRec struct: three
 * for @c r0 and three each for @c ex, @c ey, @c ez. Consumed by
 * forward-solution coil integration and by sensor visualization to draw
 * oriented coil glyphs in the device frame.
 */
class FIFFSHARED_EXPORT FiffChPos
{
public:
    using SPtr = QSharedPointer<FiffChPos>;            /**< Shared pointer type for FiffChPos. */
    using ConstSPtr = QSharedPointer<const FiffChPos>; /**< Const shared pointer type for FiffChPos. */
    using UPtr = std::unique_ptr<FiffChPos>;             /**< Unique pointer type for FiffChPos. */
    using ConstUPtr = std::unique_ptr<const FiffChPos>;  /**< Const unique pointer type for FiffChPos. */

    //=========================================================================================================
    /**
     * Constructs the coil position descriptor.
     */
    FiffChPos();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffChPos  Coil position descriptor which should be copied.
     */
    FiffChPos(const FiffChPos &p_FiffChPos);

    //=========================================================================================================
    /**
     * Destroys the coil position descriptor.
     */
    ~FiffChPos();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffChPosRec) 13*int = 13*4 = 52
     *
     * @return the size of the old struct fiffChPosRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffChPos &a, const FiffChPos &b);

public:
    fiff_int_t   coil_type; /**< What kind of coil. */
    Eigen::Vector3f r0;     /**< Coil coordinate system origin. */
    Eigen::Vector3f ex;     /**< Coil coordinate system x-axis unit vector. */
    Eigen::Vector3f ey;     /**< Coil coordinate system y-axis unit vector. */
    Eigen::Vector3f ez;     /**< Coil coordinate system z-axis unit vector. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffChPos::storageSize()
{
    return sizeof(FiffChPos::coil_type)
         + sizeof(FiffChPos::r0) + sizeof(FiffChPos::ex)
         + sizeof(FiffChPos::ey) + sizeof(FiffChPos::ez);
}

//=============================================================================================================

inline bool operator== (const FiffChPos &a, const FiffChPos &b)
{
    return (a.coil_type == b.coil_type &&
            a.r0.isApprox(b.r0, 0.0001f) &&
            a.ex.isApprox(b.ex, 0.0001f) &&
            a.ey.isApprox(b.ey, 0.0001f) &&
            a.ez.isApprox(b.ez, 0.0001f));
}
} // NAMESPACE

#endif // FIFF_CH_POS_H
